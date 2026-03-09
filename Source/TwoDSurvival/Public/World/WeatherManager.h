// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/TimeManager.h"   // ESeason
#include "WeatherManager.generated.h"

class UExponentialHeightFogComponent;
class UAudioComponent;
class USoundBase;
class ATimeManager;

UENUM(BlueprintType)
enum class EWeatherState : uint8
{
	Clear     UMETA(DisplayName = "Clear"),
	Cloudy    UMETA(DisplayName = "Cloudy"),
	Rain      UMETA(DisplayName = "Rain"),
	HeavyRain UMETA(DisplayName = "Heavy Rain"),
	Snow      UMETA(DisplayName = "Snow"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EWeatherState, NewState);

/**
 * Tick-driven weather state machine. Place one BP_WeatherManager in each persistent level.
 *
 * The manager selects the next state based on the current season (from ATimeManager) using
 * weighted random rolls. It pushes fog and tint overrides to ATimeManager so both day/night
 * and weather blend cleanly through the same components.
 *
 * Blueprint child (BP_WeatherManager):
 *   - Assign SFX_Rain / SFX_HeavyRain / SFX_Snow sound assets in the Details panel.
 *   - No components to add — audio is created at runtime.
 */
UCLASS()
class TWODSURVIVAL_API AWeatherManager : public AActor
{
	GENERATED_BODY()

public:
	AWeatherManager();

	// --- State duration ---

	/** Minimum real-world seconds a weather state lasts before rolling the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MinStateDurationSeconds = 120.f;

	/** Maximum real-world seconds a weather state can last. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MaxStateDurationSeconds = 600.f;

	// --- Ambient sounds (assign in BP Details panel) ---

	UPROPERTY(EditAnywhere, Category = "Weather|Sound")
	TObjectPtr<USoundBase> SFX_Rain;

	UPROPERTY(EditAnywhere, Category = "Weather|Sound")
	TObjectPtr<USoundBase> SFX_HeavyRain;

	UPROPERTY(EditAnywhere, Category = "Weather|Sound")
	TObjectPtr<USoundBase> SFX_Snow;

	// --- Fog density added to TimeManager's base day/night fog on each state ---

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	float CloudyFogAdd = 0.01f;

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	float RainFogAdd = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	float HeavyRainFogAdd = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	float SnowFogAdd = 0.025f;

	// --- Scene tint (multiplied with TimeManager's day/night tint) ---

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	FLinearColor CloudyTint = FLinearColor(0.9f, 0.9f, 0.95f);

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	FLinearColor RainTint = FLinearColor(0.75f, 0.82f, 0.92f);

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	FLinearColor HeavyRainTint = FLinearColor(0.55f, 0.65f, 0.82f);

	UPROPERTY(EditAnywhere, Category = "Weather|Visuals")
	FLinearColor SnowTint = FLinearColor(0.9f, 0.93f, 1.0f);

	// --- Needs modifiers (applied to outdoor players each tick) ---

	/** Thirst restored per second when outdoors in rain (player stays less thirsty). */
	UPROPERTY(EditAnywhere, Category = "Weather|Needs")
	float RainThirstBoost = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Weather|Needs")
	float HeavyRainThirstBoost = 0.04f;

	/** Extra mood drained per second when outdoors in bad weather. */
	UPROPERTY(EditAnywhere, Category = "Weather|Needs")
	float RainMoodDrain = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Weather|Needs")
	float HeavyRainMoodDrain = 0.04f;

	UPROPERTY(EditAnywhere, Category = "Weather|Needs")
	float SnowMoodDrain = 0.01f;

	// --- Runtime state (read-only) ---

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EWeatherState CurrentWeather = EWeatherState::Clear;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	bool bIsRaining = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	bool bIsSnowing = false;

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Weather")
	FOnWeatherChanged OnWeatherChanged;

	// --- Public API ---

	/** Current thirst-restore boost rate (>0 when raining outdoors). */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	float GetThirstBoostRate() const;

	/** Current extra mood drain rate (>0 in bad weather). */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	float GetMoodDrainRate() const;

	/** Elapsed seconds in the current state — used by save/load. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	float GetStateElapsedTime() const { return StateElapsedTime; }

	/** Restore weather state after loading a save. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void RestoreWeather(EWeatherState State, float ElapsedTime);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientAudioComp;

	UPROPERTY()
	TObjectPtr<ATimeManager> CachedTimeManager;

	float StateDuration    = 0.f;
	float StateElapsedTime = 0.f;

	void SelectNextWeatherState();
	void ApplyWeatherToTimeManager();
	void UpdateNeedsModifiers();
	void PlayWeatherSound(USoundBase* Sound);

	static EWeatherState PickWeightedState(ESeason Season);
};
