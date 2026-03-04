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
	if (TimeOfDay >= 1.f) TimeOfDay -= 1.f;

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
		CachedHeightFog->SetFogDensity(FMath::Lerp(NightFogDensity, DayFogDensity, Solar));
	}

	if (CachedPostProcess)
	{
		CachedPostProcess->Settings.SceneColorTint = FLinearColor::LerpUsingHSV(NightTint, DayTint, Solar);
		CachedPostProcess->Settings.bOverride_SceneColorTint = true;
	}
}
