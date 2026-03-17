// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WorldProp.h"
#include "Interaction/InteractionBehaviorComponent.h"
#include "Interaction/BreakableComponent.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AWorldProp::AWorldProp()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
}

UInteractionBehaviorComponent* AWorldProp::GetActiveBehavior() const
{
	TArray<UInteractionBehaviorComponent*> Behaviors;
	GetComponents<UInteractionBehaviorComponent>(Behaviors);

	UInteractionBehaviorComponent* Best = nullptr;
	for (UInteractionBehaviorComponent* B : Behaviors)
	{
		if (B && B->IsAvailable())
		{
			if (!Best || B->Priority > Best->Priority)
				Best = B;
		}
	}
	return Best;
}

EInteractionType AWorldProp::GetInteractionType_Implementation()
{
	if (UInteractionBehaviorComponent* B = GetActiveBehavior())
		return B->GetInteractionType();
	return EInteractionType::Instant;
}

float AWorldProp::GetInteractionDuration_Implementation()
{
	if (UInteractionBehaviorComponent* B = GetActiveBehavior())
		return B->GetInteractionDuration();
	return 0.f;
}

FText AWorldProp::GetInteractionPrompt_Implementation()
{
	if (UInteractionBehaviorComponent* B = GetActiveBehavior())
		return B->GetPrompt();
	return FText::GetEmpty();
}

void AWorldProp::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (UInteractionBehaviorComponent* B = GetActiveBehavior())
		B->Execute(Interactor);
}

void AWorldProp::TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource)
{
	if (UBreakableComponent* BC = FindComponentByClass<UBreakableComponent>())
		BC->ApplyDamage(Amount, Cast<ABaseCharacter>(DamageSource));
}
