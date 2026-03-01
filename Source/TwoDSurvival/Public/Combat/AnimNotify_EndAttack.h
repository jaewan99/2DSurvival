// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EndAttack.generated.h"

/**
 * Place this notify on an attack montage at the frame where the weapon should STOP dealing damage.
 * It calls EquippedWeapon->EndAttack() which disables the hitbox and clears the auto-close timer.
 *
 * This gives frame-accurate hitbox close when paired with AnimNotify_BeginAttack.
 * If this notify is absent, the hitbox closes automatically after AWeaponBase::SwingWindowDuration.
 */
UCLASS()
class TWODSURVIVAL_API UAnimNotify_EndAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
