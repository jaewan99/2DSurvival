// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractionBehaviorComponent.h"
#include "Character/BaseCharacter.h"
#include "Weapon/WeaponBase.h"

FText UInteractionBehaviorComponent::GetPrompt() const
{
	return FText::FromString(TEXT("Interact"));
}

bool UInteractionBehaviorComponent::IsAvailable() const
{
	return true;
}

bool UInteractionBehaviorComponent::CanInteract(ABaseCharacter* Interactor) const
{
	if (RequiredTool == EToolType::None) return true;
	return GetEquippedToolType(Interactor) == RequiredTool;
}

void UInteractionBehaviorComponent::Execute(ABaseCharacter* Interactor)
{
}

EInteractionType UInteractionBehaviorComponent::GetInteractionType() const
{
	return EInteractionType::Instant;
}

float UInteractionBehaviorComponent::GetInteractionDuration() const
{
	return 0.f;
}

EToolType UInteractionBehaviorComponent::GetEquippedToolType(ABaseCharacter* Interactor) const
{
	if (!Interactor || !Interactor->EquippedWeapon) return EToolType::None;
	return Interactor->EquippedWeapon->ToolType;
}
