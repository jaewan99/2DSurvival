// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

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

	// Hitbox used for attack collision. Disabled by default — enabled during attack animations.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> HitboxComponent;

	// Base damage dealt by this weapon.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseDamage = 10.f;
};
