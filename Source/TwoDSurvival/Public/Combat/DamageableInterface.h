// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this on any actor that can receive melee damage (enemies, destructibles, etc.).
 * The weapon's HitboxComponent overlap handler calls TakeMeleeDamage when it hits something.
 */
class TWODSURVIVAL_API IDamageable
{
	GENERATED_BODY()

public:
	/**
	 * Called when a melee weapon hitbox overlaps this actor.
	 * @param Amount        Damage after weapon BaseDamage * arm health multiplier.
	 * @param DamageSource  The character that initiated the attack.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void TakeMeleeDamage(float Amount, AActor* DamageSource);
};
