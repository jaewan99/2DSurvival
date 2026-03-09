// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlashlightActor.generated.h"

class USpotLightComponent;
class UItemDefinition;

/**
 * Equippable flashlight that attaches to the character's FlashlightSocket.
 * Spawned by ABaseCharacter::EquipItem when the item has bIsFlashlight=true.
 *
 * Blueprint child (BP_FlashlightActor):
 *   - No setup needed — SpotLight is created in C++.
 *   - Tune InnerConeAngle / OuterConeAngle / Intensity / AttenuationRadius in the
 *     SpotLight component's Details panel after selecting the component.
 */
UCLASS()
class TWODSURVIVAL_API AFlashlightActor : public AActor
{
	GENERATED_BODY()

public:
	AFlashlightActor();

	// The spot light cast by the flashlight.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> SpotLight;

	// How many battery units are drained per second while the light is on.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float DrainRate = 5.f;

	// Current battery level (0–100). Drains while light is on; recharged by battery items.
	UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
	float BatteryCharge = 100.f;

	// Whether the light is currently emitting.
	UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
	bool bIsLightOn = true;

	// The item definition this flashlight was spawned from — used for inventory validation.
	UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<UItemDefinition> SourceItemDef;

	/** Toggle the light on/off. No-op if battery is dead. */
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void Toggle();

	/** Restore battery by Amount (clamped to 100). */
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void RefillBattery(float Amount);

	/**
	 * Returns true if the light is on AND WorldPos is within the spot light's cone.
	 * Used by AEnemyBase to increase aggro range when the flashlight illuminates them.
	 */
	bool IsInCone(FVector WorldPos) const;

protected:
	virtual void Tick(float DeltaTime) override;
};
