// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WorldItem.h"
#include "Character/BaseCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/NeedsComponent.h"
#include "Kismet/GameplayStatics.h"

AWorldItem::AWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// Box is the root — InteractionComponent on the player detects overlap with this
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	RootComponent = InteractionBox;
	InteractionBox->SetBoxExtent(FVector(40.f, 40.f, 40.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAll"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.3f));
}

void AWorldItem::BeginPlay()
{
	Super::BeginPlay();

	// If this actor spawns inside an already-active InteractionComponent detection sphere,
	// OnComponentBeginOverlap won't fire automatically. Force a recheck so nearby players
	// pick it up immediately without having to walk away and back.
	if (InteractionBox)
	{
		InteractionBox->UpdateOverlaps();
	}
}

// --- IInteractable ---

EInteractionType AWorldItem::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float AWorldItem::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText AWorldItem::GetInteractionPrompt_Implementation()
{
	if (!ItemDef) return FText::FromString(TEXT("Pick up"));

	if (Quantity > 1)
	{
		return FText::FromString(
			FString::Printf(TEXT("Pick up %s (x%d)"), *ItemDef->DisplayName.ToString(), Quantity));
	}
	return FText::Format(FText::FromString(TEXT("Pick up {0}")), ItemDef->DisplayName);
}

void AWorldItem::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor || !ItemDef) return;

	UInventoryComponent* Inv = Interactor->InventoryComponent;
	if (!Inv) return;

	if (Inv->TryAddItem(ItemDef, Quantity))
	{
		if (Interactor->NeedsComponent)
			Interactor->NeedsComponent->ModifyMood(3.f);
		UGameplayStatics::PlaySoundAtLocation(this, SFX_Pickup, GetActorLocation());
		Destroy();
	}
}
