// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeedsComponent.generated.h"

class ABaseCharacter;

UENUM(BlueprintType)
enum class ENeedType : uint8
{
	Hunger  UMETA(DisplayName = "Hunger"),
	Thirst  UMETA(DisplayName = "Thirst"),
	Fatigue UMETA(DisplayName = "Fatigue"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnNeedChanged,
	ENeedType, NeedType, float, CurrentValue, float, MaxValue, bool, bIsWarning);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoodChanged, float, NewMood);

/**
 * Tick-driven component that drains Hunger, Thirst, and Fatigue over time.
 * At 0 each need causes HP drain. Low values apply speed and damage penalties.
 * Sleeping state restores Fatigue instead of draining it.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UNeedsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNeedsComponent();

	/** Broadcast whenever any need value changes. */
	UPROPERTY(BlueprintAssignable, Category = "Needs")
	FOnNeedChanged OnNeedChanged;

	/** Broadcast whenever the Mood value changes. */
	UPROPERTY(BlueprintAssignable, Category = "Needs")
	FOnMoodChanged OnMoodChanged;

	/** Very slow passive Mood drain (~208 min to empty at 1x). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Mood")
	float MoodPassiveDrainRate = 0.008f;

	/** Extra Mood drain per depleted (0) need. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Mood")
	float CriticalMoodDrainRate = 0.04f;

	/** Adjust Mood by Delta (clamped 0–100) and broadcast OnMoodChanged. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void ModifyMood(float Delta);

	/** Returns current Mood (0–100). */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	float GetMood() const;

	/** Directly set Mood value — used by the save/load system. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void SetMood(float Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Drain")
	float HungerDrainRate = 0.055f;   // ~30 min to fully deplete

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Drain")
	float ThirstDrainRate = 0.075f;   // ~22 min to fully deplete

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Drain")
	float FatigueDrainRate = 0.04f;   // ~41 min to fully deplete

	/** Drain rate multiplier when the character is running or attacking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs|Drain")
	float ActiveMultiplier = 2.0f;

	/** Need value below which warning icons appear and penalties kick in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	float WarningThreshold = 50.f;

	/** HP drained per second per depleted (0) need — applied to EBodyPart::Body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	float CriticalHPDrain = 0.5f;

	/**
	 * When true, Fatigue restores (at 3x the drain rate) instead of draining.
	 * Auto-resets and calls OwnerChar->StopSleeping() when Fatigue reaches 100.
	 */
	bool bIsSleeping = false;

	/** Fatigue value recorded the moment sleep begins. Auto-wake only fires if this
	 *  was below 95 — prevents immediate wake when the player sleeps with near-full fatigue. */
	float SleepStartFatigue = 0.f;

	/**
	 * Set by BaseCharacter::StartSleeping to match the world TimeScale.
	 * Fatigue restoration is multiplied by this so it fills proportionally to
	 * how fast in-game time is passing. Reset to 1 on StopSleeping.
	 */
	float SleepTimeScaleMultiplier = 1.f;

	/** Restore a need by Amount. Clamped to [0, 100]. Broadcasts OnNeedChanged. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void RestoreNeed(ENeedType NeedType, float Amount);

	/** Directly set a need value — used by the save/load system. Broadcasts OnNeedChanged. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	void SetNeedValue(ENeedType NeedType, float Value);

	/** Returns the current value (0–100) of the given need. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	float GetNeedValue(ENeedType NeedType) const;

	/** 0.7 if any need is below WarningThreshold, otherwise 1.0. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	float GetSpeedMultiplier() const;

	/** 0.8 if any need is below WarningThreshold, otherwise 1.0. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	float GetDamageMultiplier() const;

	/** True if any need is below WarningThreshold — blocks health regeneration. */
	UFUNCTION(BlueprintCallable, Category = "Needs")
	bool IsHealthRegenBlocked() const;

	/** Called from BaseCharacter Tick — doubles drain when the character is moving fast or attacking. */
	void SetActiveMovement(bool bActive);

	/**
	 * True while the player is inside a building (set by ABuildingEntrance on enter/exit).
	 * When indoors, weather-driven thirst boosts and mood drains do NOT apply.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Needs")
	bool bIsIndoors = false;

	/**
	 * Called by AWeatherManager on each weather state change.
	 * ThirstBoost:  thirst restored per second while outdoors (positive = less thirsty).
	 * MoodDrain:    extra mood lost per second while outdoors in bad weather.
	 */
	void SetWeatherModifiers(float ThirstBoost, float MoodDrain);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float Hunger  = 100.f;
	float Thirst  = 100.f;
	float Fatigue = 100.f;
	float Mood    = 100.f;

	bool bIsActiveMovement = false;

	// Set by AWeatherManager on each state change. Zero when weather is benign.
	float WeatherThirstBoostRate = 0.f;
	float WeatherMoodDrainRate   = 0.f;

	UPROPERTY()
	ABaseCharacter* OwnerChar = nullptr;

	void DrainNeed(ENeedType NeedType, float DeltaTime);
	void ApplyCriticalEffects(float DeltaTime);
};
