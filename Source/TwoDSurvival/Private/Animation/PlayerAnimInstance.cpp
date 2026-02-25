// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/PlayerAnimInstance.h"
#include "Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter || !OwnerMovement) return;

	const FVector Velocity = OwnerCharacter->GetVelocity();

	// 2D game — only X axis matters for horizontal speed
	Speed         = FMath::Abs(Velocity.X);
	bIsInAir      = OwnerMovement->IsFalling();
	bIsAttacking  = OwnerCharacter->bIsAttacking;
	bIsMovingRight = Velocity.X > 0.f;
}
