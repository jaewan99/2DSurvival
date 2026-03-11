// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/StatusEffectTypes.h"
#include "HazardZone.generated.h"

class UBoxComponent;
class ABaseCharacter;
class USoundBase;
class UAudioComponent;

/**
 * AHazardZone — a placeable trigger volume that applies status effects and/or
 * direct damage to the player at a configurable interval while they're inside.
 *
 * Configure the hazard type entirely in the Blueprint child's Details panel:
 *   BP_FireHazard   — EffectsToApply=[Bleeding],  DirectDamagePerInterval=5
 *   BP_ToxicHazard  — EffectsToApply=[Poisoned],  DirectDamagePerInterval=0
 *   BP_ColdHazard   — EffectsToApply=[Wet],        DirectDamagePerInterval=0
 *
 * Effects remain active after the player leaves — they must be cured normally.
 * Ambient sound starts on player entry and stops on exit.
 */
UCLASS()
class TWODSURVIVAL_API AHazardZone : public AActor
{
	GENERATED_BODY()

public:
	AHazardZone();

	// ── Config ────────────────────────────────────────────────────────────

	/** Status effects applied (with indefinite duration) each EffectInterval while the player overlaps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	TArray<EStatusEffect> EffectsToApply;

	/**
	 * Raw HP damage dealt directly to EBodyPart::Body each EffectInterval while overlapping.
	 * Set to 0 for purely status-effect-based hazards (toxic gas, cold zone, etc.).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	float DirectDamagePerInterval = 0.f;

	/** Seconds between each application of effects and direct damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	float EffectInterval = 2.f;

	/** Optional ambient sound looped while a player is inside the zone. Assign in BP. */
	UPROPERTY(EditDefaultsOnly, Category = "Hazard|Sound")
	TObjectPtr<USoundBase> SFX_Ambient;

protected:
	virtual void BeginPlay() override;

private:
	/** Trigger volume — resize in the Blueprint viewport to match the hazard area. */
	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<UBoxComponent> HazardBox;

	/**
	 * Optional visual mesh (fire plane, gas cloud billboard, etc.).
	 * Assign the mesh asset in the Blueprint child's Details panel.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Audio component used for the looping ambient SFX. Created at construct time. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientAudio;

	/** Weak ref to the player currently inside the zone. Null when unoccupied. */
	UPROPERTY()
	TObjectPtr<ABaseCharacter> OverlappingPlayer;

	FTimerHandle EffectTimerHandle;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Called on EffectInterval — applies all configured effects and direct damage. */
	void ApplyHazardEffects();
};
