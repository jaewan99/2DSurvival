// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NoiseEmitterComponent.generated.h"

UENUM(BlueprintType)
enum class ENoiseType : uint8
{
	Footstep   UMETA(DisplayName = "Footstep"),
	Combat     UMETA(DisplayName = "Combat"),
	Door       UMETA(DisplayName = "Door"),
	ItemPickup UMETA(DisplayName = "Item Pickup"),
};

/**
 * Emits noise events that alert nearby enemies.
 * Placed on ABaseCharacter. Enemies within the noise radius transition to the Alert state.
 * Also exposes BroadcastNoiseAt() as a static utility for doors, pickups, and other world actors.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNoiseEmitterComponent();

	// Emit a noise at the owning actor's location. Finds all AEnemyBase within the noise radius.
	UFUNCTION(BlueprintCallable, Category = "Noise")
	void EmitNoise(ENoiseType Type, float RadiusOverride = 0.f);

	// Emit a noise at an arbitrary world position. Use for doors, item pickups, explosions, etc.
	// Safe to call from any actor — does not require a NoiseEmitterComponent owner.
	static void BroadcastNoiseAt(UWorld* World, FVector Origin, float Radius);

	// Tick helper: called from BaseCharacter::Tick to emit throttled footstep noise.
	// bIsMoving: whether the character has non-zero velocity. SpeedFraction: current speed / base walk speed.
	void TickFootstep(float DeltaTime, bool bIsMoving, float SpeedFraction);

	// Minimum seconds between footstep noise emissions.
	UPROPERTY(EditDefaultsOnly, Category = "Noise|Radii")
	float FootstepInterval = 0.4f;

	// Base noise radius (cm) for each noise type.
	UPROPERTY(EditDefaultsOnly, Category = "Noise|Radii")
	float FootstepRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise|Radii")
	float CombatRadius = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise|Radii")
	float DoorRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise|Radii")
	float ItemPickupRadius = 200.f;

private:
	float FootstepAccum = 0.f;

	float GetRadiusForType(ENoiseType Type) const;
};
