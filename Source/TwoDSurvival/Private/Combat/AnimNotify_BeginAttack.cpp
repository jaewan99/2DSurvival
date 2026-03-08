// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AnimNotify_BeginAttack.h"
#include "Character/BaseCharacter.h"
#include "Character/HealthComponent.h"
#include "Weapon/WeaponBase.h"
#include "Enemy/EnemyBase.h"

void UAnimNotify_BeginAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	// Player weapon path
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner()))
	{
		AWeaponBase* Weapon = Character->EquippedWeapon;
		if (!Weapon) return;

		const float DamageMultiplier = Character->HealthComponent
			? Character->HealthComponent->GetDamageMultiplier()
			: 1.f;

		Weapon->BeginAttack(DamageMultiplier);
		return;
	}

	// Enemy path — enable hitbox at the correct animation frame
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(MeshComp->GetOwner()))
	{
		Enemy->EnableMeleeHitbox();
	}
}
