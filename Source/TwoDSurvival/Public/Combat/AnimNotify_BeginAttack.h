// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BeginAttack.generated.h"

/**
 * Place this notify on an attack montage at the frame where the weapon should START dealing damage.
 * It finds the owning ABaseCharacter, reads the arm health damage multiplier, then calls
 * EquippedWeapon->BeginAttack(multiplier) which enables the hitbox and starts the auto-close timer.
 *
 * If you also place AnimNotify_EndAttack later in the montage, it will cancel the timer early
 * for frame-accurate hitbox close.  If AnimNotify_EndAttack is absent, the hitbox closes
 * automatically after AWeaponBase::SwingWindowDuration seconds.
 */
UCLASS()
class TWODSURVIVAL_API UAnimNotify_BeginAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
