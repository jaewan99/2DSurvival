// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PropInteractionTypes.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UItemDefinition;

/**
 * Base class for all equippable weapon actors.
 * Subclass in Blueprint (e.g. BP_WeaponSword) to assign a mesh and resize the hitbox.
 * Spawned and attached to a character socket when equipped via EquipItem.
 */
UCLASS()
class TWODSURVIVAL_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// The visual mesh for this weapon. Assign in Blueprint subclass.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// Hitbox used for attack collision. Disabled by default — enabled during BeginAttack.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> HitboxComponent;

	// Base damage dealt by this weapon.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage = 10.f;

	// Tool type this weapon counts as for breaking props (UBreakableComponent checks this).
	// Set to Hammer for a hammer weapon, Axe for an axe, etc. None = generic weapon.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EToolType ToolType = EToolType::None;

	// How long (seconds) the hitbox stays active per swing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float SwingWindowDuration = 0.35f;

	// Attack montage played on the character when this weapon is swung.
	// Assign in the Blueprint subclass (e.g. BP_WeaponHammer) Details panel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> AttackMontage;

	// The item definition this weapon was spawned from. Set automatically by EquipItem.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UItemDefinition> SourceItemDef;

	/**
	 * Enables the hitbox for the swing window, then auto-disables it after SwingWindowDuration.
	 * Called by the owning character (or AnimNotify_BeginAttack) when an attack starts.
	 * @param DamageMultiplier  Arm health multiplier from the character's HealthComponent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void BeginAttack(float DamageMultiplier);

	/**
	 * Disables the hitbox and clears the auto-close timer.
	 * Called automatically by the SwingWindowDuration timer, or by AnimNotify_EndAttack
	 * for frame-accurate close at a specific position in the attack montage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EndAttack();

protected:
	virtual void BeginPlay() override;

private:
	// Damage multiplier cached from BeginAttack — used in the overlap handler.
	float StoredDamageMultiplier = 1.f;

	// Tracks which actors were already hit this swing to prevent multi-hit.
	TSet<AActor*> HitActorsThisSwing;

	FTimerHandle SwingTimerHandle;

	UFUNCTION()
	void OnHitboxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
