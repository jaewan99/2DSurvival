// Fill out your copyright notice in the Description page of Project Settings.

#include "World/TimeManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"

ATimeManager::ATimeManager()
{
	PrimaryActorTick.bCanEverTick = true;
	// Components added in BP_TimeManager via Add Component (any name).
	// C++ finds them at BeginPlay via GetComponentByClass — no name coupling.
}

void ATimeManager::BeginPlay()
{
	Super::BeginPlay();

	CachedSunLight    = GetComponentByClass<UDirectionalLightComponent>();
	CachedSkyLight    = GetComponentByClass<USkyLightComponent>();
	CachedHeightFog   = GetComponentByClass<UExponentialHeightFogComponent>();
	CachedPostProcess = GetComponentByClass<UPostProcessComponent>();

	UE_LOG(LogTemp, Warning, TEXT("[TimeManager] BeginPlay — SunLight:%s | SkyLight:%s | HeightFog:%s | PostProcess:%s"),
		CachedSunLight    ? TEXT("OK") : TEXT("NULL"),
		CachedSkyLight    ? TEXT("OK") : TEXT("NULL"),
		CachedHeightFog   ? TEXT("OK") : TEXT("NULL"),
		CachedPostProcess ? TEXT("OK") : TEXT("NULL"));

	TimeOfDay = StartTimeOfDay;
	bWasNight = IsNight();
	ApplyVisuals();
}

void ATimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DayDurationSeconds <= 0.f) return;

	TimeOfDay += DeltaTime * TimeScale / DayDurationSeconds;
	if (TimeOfDay >= 1.f)
	{
		TimeOfDay -= 1.f;
		AdvanceDay();
	}

	const bool bIsNightNow = IsNight();
	if (bIsNightNow != bWasNight)
	{
		bWasNight = bIsNightNow;
		UE_LOG(LogTemp, Warning, TEXT("[TimeManager] Phase changed — %s (Hour: %.1f)"),
			bIsNightNow ? TEXT("NIGHT") : TEXT("DAY"), GetHour());
		OnDayPhaseChanged.Broadcast(bIsNightNow);
	}

	static float LogAccum = 0.f;
	LogAccum += DeltaTime;
	if (LogAccum >= 2.f)
	{
		LogAccum = 0.f;
		UE_LOG(LogTemp, Log, TEXT("[TimeManager] TimeOfDay: %.3f | Hour: %.1f | %s"),
			TimeOfDay, GetHour(), IsNight() ? TEXT("Night") : TEXT("Day"));
	}

	ApplyVisuals();
}

bool ATimeManager::IsNight() const
{
	return TimeOfDay < SunriseTime || TimeOfDay > SunsetTime;
}

float ATimeManager::GetHour() const
{
	return TimeOfDay * 24.f;
}

void ATimeManager::SetTimeScale(float NewScale)
{
	TimeScale = FMath::Max(0.f, NewScale);
}

void ATimeManager::AdvanceDay()
{
	CurrentDay++;
	if (CurrentDay > DaysPerMonth)
	{
		CurrentDay = 1;
		CurrentMonth++;
		if (CurrentMonth > 12)
		{
			CurrentMonth = 1;
			CurrentYear++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[TimeManager] New day — Day %d, Month %d, Year %d (%s)"),
		CurrentDay, CurrentMonth, CurrentYear, *GetDateText().ToString());
	OnDayChanged.Broadcast(CurrentDay, CurrentMonth, CurrentYear, GetCurrentSeason());
}

ESeason ATimeManager::GetCurrentSeason() const
{
	if (CurrentMonth <= 3)  return ESeason::Spring;
	if (CurrentMonth <= 6)  return ESeason::Summer;
	if (CurrentMonth <= 9)  return ESeason::Fall;
	return ESeason::Winter;
}

int32 ATimeManager::GetDayOfYear() const
{
	return (CurrentMonth - 1) * DaysPerMonth + CurrentDay;
}

FText ATimeManager::GetDateText() const
{
	static const TCHAR* SeasonNames[] = { TEXT("Spring"), TEXT("Summer"), TEXT("Fall"), TEXT("Winter") };
	return FText::FromString(FString::Printf(TEXT("Day %d, Month %d (%s), Year %d"),
		CurrentDay, CurrentMonth, SeasonNames[(int32)GetCurrentSeason()], CurrentYear));
}

void ATimeManager::SetCalendar(int32 Day, int32 Month, int32 Year)
{
	CurrentDay   = FMath::Max(1, Day);
	CurrentMonth = FMath::Clamp(Month, 1, 12);
	CurrentYear  = FMath::Max(1, Year);
}

void ATimeManager::SetWeatherFogOffset(float Offset)
{
	WeatherFogOffset = Offset;
}

void ATimeManager::SetWeatherTint(FLinearColor Tint)
{
	WeatherTintMultiplier = Tint;
}

void ATimeManager::SetDaylight(float NewSunriseTime, float NewSunsetTime)
{
	SunriseTime = FMath::Clamp(NewSunriseTime, 0.f, 1.f);
	SunsetTime  = FMath::Clamp(NewSunsetTime,  0.f, 1.f);
}

float ATimeManager::GetSolarElevation() const
{
	if (TimeOfDay <= SunriseTime || TimeOfDay >= SunsetTime) return 0.f;

	// Normalize 0-1 across the day window, then sin-arc peaks at 1.0 at solar noon.
	const float DayProgress = (TimeOfDay - SunriseTime) / (SunsetTime - SunriseTime);
	return FMath::Sin(DayProgress * PI);
}

FLinearColor ATimeManager::GetSunColor() const
{
	if (TimeOfDay <= SunriseTime || TimeOfDay >= SunsetTime)
		return FLinearColor::Black;

	const float NoonTime = (SunriseTime + SunsetTime) * 0.5f;

	if (TimeOfDay <= NoonTime)
	{
		// Sunrise → Noon
		const float T = (TimeOfDay - SunriseTime) / (NoonTime - SunriseTime);
		return FLinearColor::LerpUsingHSV(SunriseColor, NoonColor, FMath::SmoothStep(0.f, 1.f, T));
	}
	else
	{
		// Noon → Sunset
		const float T = (TimeOfDay - NoonTime) / (SunsetTime - NoonTime);
		return FLinearColor::LerpUsingHSV(NoonColor, SunsetColor, FMath::SmoothStep(0.f, 1.f, T));
	}
}

void ATimeManager::ApplyVisuals()
{
	const float Solar = GetSolarElevation();

	if (CachedSunLight)
	{
		CachedSunLight->SetIntensity(Solar * MaxSunIntensity);
		CachedSunLight->SetLightColor(GetSunColor());
	}

	if (CachedSkyLight)
	{
		CachedSkyLight->SetIntensity(FMath::Lerp(NightSkyIntensity, DaySkyIntensity, Solar));
	}

	if (CachedHeightFog)
	{
		CachedHeightFog->SetFogDensity(FMath::Lerp(NightFogDensity, DayFogDensity, Solar) + WeatherFogOffset);
	}

	if (CachedPostProcess)
	{
		const FLinearColor BaseTint = FLinearColor::LerpUsingHSV(NightTint, DayTint, Solar);
		CachedPostProcess->Settings.SceneColorTint = BaseTint * WeatherTintMultiplier;
		CachedPostProcess->Settings.bOverride_SceneColorTint = true;
	}
}
