// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/StatusEffectTypes.h"
#include "StatusEffectComponent.generated.h"

class UNeedsComponent;
class AWeatherManager;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusEffectsChanged);

/**
 * Manages all active status effects on the player.
 * Ticks each effect, applies per-second consequences, and handles automatic progressions:
 *   Bleeding (120 s) → Infected
 *   Wet + Snow outdoors (60 s) → Frostbite
 *   Frostbite + Snow outdoors (120 s) → Hypothermia
 *
 * Wet is auto-applied while the player is outdoors in rain/snow and fades after
 * WetDryTime seconds once they go indoors or the rain stops.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	// ── Effect config (tune in BP_BaseCharacter) ──────────────────────────

	/** HP/s drained from Body while Bleeding is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float BleedDrainRate = 0.3f;

	/** Seconds of untreated Bleeding before Infected is automatically applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float BleedToInfectTime = 120.f;

	/** HP/s drained from Body while Infected is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float InfectDrainRate = 0.1f;

	/** HP/s drained from Body while Poisoned is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float PoisonDrainRate = 0.2f;

	/** HP/s drained from Body while Hypothermia is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float HypothermiaDrainRate = 0.15f;

	/** Seconds to fully dry off after going indoors or rain stopping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float WetDryTime = 60.f;

	/** Seconds of outdoor snow exposure (or Wet+Snow) before Frostbite is applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float FrostbiteThreshold = 60.f;

	/** Seconds of Frostbite in cold conditions before Hypothermia is applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StatusEffects|Config")
	float HypothermiaThreshold = 120.f;

	// ── Runtime state ─────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "StatusEffects")
	TArray<FActiveStatusEffect> ActiveEffects;

	UPROPERTY(BlueprintAssignable, Category = "StatusEffects")
	FOnStatusEffectsChanged OnStatusEffectsChanged;

	// ── Public API ────────────────────────────────────────────────────────

	/**
	 * Apply a status effect. If the effect is already active and the new severity
	 * is higher, the existing entry is replaced. Otherwise it is left unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "StatusEffects")
	void ApplyEffect(EStatusEffect Effect, float Severity = 1.f, float Duration = -1.f);

	/** Remove a status effect entirely. Safe to call when the effect is not active. */
	UFUNCTION(BlueprintCallable, Category = "StatusEffects")
	void RemoveEffect(EStatusEffect Effect);

	/** Returns true if the given effect is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StatusEffects")
	bool HasEffect(EStatusEffect Effect) const;

	/**
	 * Combined movement speed multiplier from all active effects.
	 * BrokenBone 0.5 × Frostbite 0.85 × Hypothermia 0.7 × Concussion 0.8
	 * Returns 1.0 when no effects are active.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StatusEffects")
	float GetSpeedMultiplier() const;

	/**
	 * Combined weapon damage multiplier from all active effects.
	 * Frostbite 0.8 × Poisoned 0.9 × Concussion 0.7
	 * Returns 1.0 when no effects are active.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StatusEffects")
	float GetDamageMultiplier() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Cached component/actor references set in BeginPlay.
	UPROPERTY()
	TObjectPtr<UNeedsComponent> CachedNeeds;

	UPROPERTY()
	TObjectPtr<UHealthComponent> CachedHealth;

	UPROPERTY()
	TObjectPtr<AWeatherManager> CachedWeather;

	// Progression accumulators — NOT saved (re-derive from world state on load).
	float BleedingTimer     = 0.f;  // Tracks untreated Bleeding duration → Infected
	float ColdExposureAccum = 0.f;  // Outdoor snow exposure → Frostbite
	float HypothermiaAccum  = 0.f;  // Active Frostbite in cold → Hypothermia

	FActiveStatusEffect* GetEffectPtr(EStatusEffect Effect);
};
