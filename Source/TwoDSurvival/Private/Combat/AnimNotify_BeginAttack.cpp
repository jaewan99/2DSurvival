// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AnimNotify_BeginAttack.h"
#include "Character/BaseCharacter.h"
#include "Character/HealthComponent.h"
#include "Weapon/WeaponBase.h"

void UAnimNotify_BeginAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character) return;

	// Only relevant when the player has a weapon equipped
	AWeaponBase* Weapon = Character->EquippedWeapon;
	if (!Weapon) return;

	const float DamageMultiplier = Character->HealthComponent
		? Character->HealthComponent->GetDamageMultiplier()
		: 1.f;

	Weapon->BeginAttack(DamageMultiplier);
}
