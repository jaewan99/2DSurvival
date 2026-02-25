// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

class ABaseCharacter;
class UCharacterMovementComponent;

/**
 * Custom AnimInstance for the player character.
 * All animation variables are calculated here in C++ and read by the AnimBP AnimGraph.
 * Blueprint child (ABP_Player) only needs the visual AnimGraph — no Blueprint logic.
 */
UCLASS()
class TWODSURVIVAL_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Horizontal movement speed — drives Idle/Walk transition.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed = 0.f;

	// True while the character is airborne — drives Jump state.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInAir = false;

	// True while an attack montage is playing — use to disable foot IK in the AnimGraph.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAttacking = false;

	// True when the character is moving right (positive X velocity) — for sprite/mesh flipping if needed.
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsMovingRight = false;

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> OwnerMovement;
};
