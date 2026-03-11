// Fill out your copyright notice in the Description page of Project Settings.

#include "World/PlaceableActor.h"
#include "Character/BaseCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"

APlaceableActor::APlaceableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// Default half-extent; Blueprint children can resize to match the mesh footprint.
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
}

void APlaceableActor::BeginPlay()
{
	Super::BeginPlay();
}

// ── Ghost mode ────────────────────────────────────────────────────────────────

void APlaceableActor::SetGhostMode(bool bGhost, UMaterialInterface* GhostMaterial)
{
	bIsGhost = bGhost;

	if (bGhost)
	{
		// No collision while previewing — don't block movement or trigger interaction.
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Override all material slots with the ghost material so the actor appears translucent.
		if (GhostMaterial)
		{
			for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
			{
				Mesh->SetMaterial(i, GhostMaterial);
			}
		}
	}
	else
	{
		// Restore normal collision for the placed actor.
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// Clear material overrides so the mesh shows its original materials.
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Mesh->SetMaterial(i, nullptr);
		}
	}
}

void APlaceableActor::SetGhostValid(bool bValid,
	UMaterialInterface* ValidMaterial,
	UMaterialInterface* InvalidMaterial)
{
	if (!bIsGhost) return;

	UMaterialInterface* Mat = bValid ? ValidMaterial : InvalidMaterial;
	if (!Mat) return;

	for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
	{
		Mesh->SetMaterial(i, Mat);
	}
}

void APlaceableActor::FinalizePlace(UItemDefinition* ItemDef)
{
	SourceItemDef = ItemDef;
	PlacementID   = FGuid::NewGuid();
	SetGhostMode(false);
}

// ── IInteractable ─────────────────────────────────────────────────────────────

EInteractionType APlaceableActor::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float APlaceableActor::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText APlaceableActor::GetInteractionPrompt_Implementation()
{
	if (SourceItemDef)
	{
		return FText::Format(
			NSLOCTEXT("PlaceableActor", "PickUpPrompt", "Pick Up {0}"),
			SourceItemDef->DisplayName);
	}
	return NSLOCTEXT("PlaceableActor", "PickUpFallback", "Pick Up");
}

void APlaceableActor::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor || !SourceItemDef) return;

	UInventoryComponent* Inv = Interactor->InventoryComponent;
	if (!Inv) return;

	if (Inv->TryAddItem(SourceItemDef, 1))
	{
		UE_LOG(LogTemp, Log, TEXT("[PlaceableActor] Picked up '%s'."),
			*SourceItemDef->DisplayName.ToString());
		Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PlaceableActor] Cannot pick up '%s' — inventory is full."),
			*SourceItemDef->DisplayName.ToString());
	}
}
