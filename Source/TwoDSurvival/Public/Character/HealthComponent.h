// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/HealthTypes.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBodyPartDamaged,
	EBodyPart, Part, float, CurrentHealth, float, MaxHealth, bool, bJustBroken);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TWODSURVIVAL_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// Fires whenever a body part takes damage.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnBodyPartDamaged OnBodyPartDamaged;

	// Fires when Head or Body reaches 0.
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

	// Per-part max health — configure in BP defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float HeadMaxHealth = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float BodyMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float ArmMaxHealth = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float LegMaxHealth = 75.f;

	// Apply damage to a body part. Clamps to 0, fires delegates.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(EBodyPart Part, float Amount);

	// Restore health on a body part. Clamps to MaxHealth.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void RestoreHealth(EBodyPart Part, float Amount);

	// Returns current health as a 0–1 fraction for the given part.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthPercent(EBodyPart Part) const;

	// Returns a copy of the FBodyPartHealth struct for the given part.
	UFUNCTION(BlueprintCallable, Category = "Health")
	FBodyPartHealth GetBodyPart(EBodyPart Part) const;

	// True if Head or Body has reached 0.
	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const;

	// Average of arm health percentages — multiplied against weapon BaseDamage.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetDamageMultiplier() const;

	// 1.0 both legs fine, 0.75 one broken, 0.25 both broken.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMovementSpeedMultiplier() const;

	// Directly set a body part's current health. Used by the save/load system.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetBodyPartHealth(EBodyPart Part, float NewCurrentHealth);

protected:
	virtual void BeginPlay() override;

private:
	// Runtime state for every body part.
	TMap<EBodyPart, FBodyPartHealth> BodyParts;
};
