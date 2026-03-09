// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeManager.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayPhaseChanged, bool, bIsNight);

UENUM(BlueprintType)
enum class ESeason : uint8
{
	Spring UMETA(DisplayName = "Spring"),
	Summer UMETA(DisplayName = "Summer"),
	Fall   UMETA(DisplayName = "Fall"),
	Winter UMETA(DisplayName = "Winter"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDayChanged, int32, Day, int32, Month, int32, Year, ESeason, Season);

/**
 * World-level day/night cycle manager.
 * All visuals are computed from C++ math — no curve assets needed.
 * SunriseTime / SunsetTime are runtime-settable so climate can shift day length.
 * In BP_TimeManager, add Directional Light / Sky Light / Exponential Height Fog /
 * Post Process components — name them anything, C++ finds them by type.
 */
UCLASS()
class TWODSURVIVAL_API ATimeManager : public AActor
{
	GENERATED_BODY()

public:
	ATimeManager();

	// --- Time config ---

	// Total real-world seconds for one full day. Default = 600 (10 minutes).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float DayDurationSeconds = 600.f;

	// Time of day on load. 0=midnight, 0.25=6AM, 0.5=noon, 0.75=6PM.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float StartTimeOfDay = 0.25f;

	// Normalized time when sun rises (default 0.25 = 6 AM).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float SunriseTime = 0.25f;

	// Normalized time when sun sets (default 0.75 = 6 PM).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float SunsetTime = 0.75f;

	// --- Sun (directional light) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sun")
	float MaxSunIntensity = 10.f;

	// Color at sunrise / sunset horizon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sun")
	FLinearColor SunriseColor = FLinearColor(1.0f, 0.45f, 0.1f);

	// Color at solar noon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sun")
	FLinearColor NoonColor = FLinearColor(1.0f, 0.95f, 0.85f);

	// Color at sunset (can differ from sunrise for variety).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sun")
	FLinearColor SunsetColor = FLinearColor(1.0f, 0.3f, 0.05f);

	// --- Sky light ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sky")
	float DaySkyIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Sky")
	float NightSkyIntensity = 0.05f;

	// --- Height fog ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Fog")
	float DayFogDensity = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|Fog")
	float NightFogDensity = 0.02f;

	// --- Post process tint ---

	// Scene tint at noon (usually white).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|PostProcess")
	FLinearColor DayTint = FLinearColor::White;

	// Scene tint at midnight — cool blue moonlight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting|PostProcess")
	FLinearColor NightTint = FLinearColor(0.4f, 0.5f, 0.9f, 1.f);

	// --- Calendar ---

	/** Number of in-game days per month. 3 months per season, 4 seasons = 12 months per year. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar")
	int32 DaysPerMonth = 30;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
	int32 CurrentDay = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
	int32 CurrentMonth = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
	int32 CurrentYear = 1;

	// --- Runtime state ---

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	float TimeOfDay = 0.25f;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category = "Time")
	bool IsNight() const;

	UFUNCTION(BlueprintCallable, Category = "Time")
	float GetHour() const;

	// Called by the climate system to shift day length at runtime.
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetDaylight(float NewSunriseTime, float NewSunsetTime);

	UFUNCTION(BlueprintCallable, Category = "Calendar")
	ESeason GetCurrentSeason() const;

	/** Day 1 of month 1 = day 1; rolls over each year. */
	UFUNCTION(BlueprintCallable, Category = "Calendar")
	int32 GetDayOfYear() const;

	UFUNCTION(BlueprintCallable, Category = "Calendar")
	FText GetDateText() const;

	/** Restore calendar from a save game. */
	UFUNCTION(BlueprintCallable, Category = "Calendar")
	void SetCalendar(int32 Day, int32 Month, int32 Year);

	/**
	 * Weather fog offset added on top of the day/night lerped density.
	 * Set each weather-state change by AWeatherManager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeatherFogOffset(float Offset);

	/**
	 * Weather tint multiplied with the final scene color tint from the day/night lerp.
	 * White = no change. Set each weather-state change by AWeatherManager.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeatherTint(FLinearColor Tint);

	/**
	 * Multiplier applied to time progression each tick.
	 * 1.0 = normal speed. 20.0 = time passes 20x faster (used during sleep).
	 * Tunable in BP_TimeManager Details or set at runtime via SetTimeScale.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float TimeScale = 1.f;

	/** Set the time scale at runtime (called by BaseCharacter on sleep/wake). */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimeScale(float NewScale);

	UPROPERTY(BlueprintAssignable, Category = "Time")
	FOnDayPhaseChanged OnDayPhaseChanged;

	/** Fired once per in-game day, after midnight wrap. */
	UPROPERTY(BlueprintAssignable, Category = "Calendar")
	FOnDayChanged OnDayChanged;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UDirectionalLightComponent>     CachedSunLight;
	UPROPERTY() TObjectPtr<USkyLightComponent>             CachedSkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> CachedHeightFog;
	UPROPERTY() TObjectPtr<UPostProcessComponent>          CachedPostProcess;

	bool bWasNight = false;

	// Added by AWeatherManager on each weather state change.
	float WeatherFogOffset = 0.f;
	FLinearColor WeatherTintMultiplier = FLinearColor::White;

	void AdvanceDay();

	// 0 = fully night, 1 = solar noon. Uses sin arc between sunrise and sunset.
	float GetSolarElevation() const;

	// Sun color blended sunrise → noon → sunset.
	FLinearColor GetSunColor() const;

	void ApplyVisuals();
};
