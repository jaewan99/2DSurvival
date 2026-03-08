// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AnimNotify_EndAttack.h"
#include "Character/BaseCharacter.h"
#include "Weapon/WeaponBase.h"
#include "Enemy/EnemyBase.h"

void UAnimNotify_EndAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	// Player weapon path
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner()))
	{
		AWeaponBase* Weapon = Character->EquippedWeapon;
		if (!Weapon) return;

		// Close the hitbox immediately and cancel the auto-close timer
		Weapon->EndAttack();
		return;
	}

	// Enemy path — close hitbox at the correct animation frame
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
	{
		Enemy->DisableMeleeHitbox();
	}
}
