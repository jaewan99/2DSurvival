// Fill out your copyright notice in the Description page of Project Settings.

#include "World/CraftingTable.h"
#include "Character/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

ACraftingTable::ACraftingTable()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Box overlap — always detects the player regardless of mesh collision settings.
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(100.f, 100.f, 80.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void ACraftingTable::BeginPlay()
{
	Super::BeginPlay();
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ACraftingTable::OnBoxEndOverlap);
}

// --- IInteractable ---

EInteractionType ACraftingTable::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float ACraftingTable::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText ACraftingTable::GetInteractionPrompt_Implementation()
{
	return FText::FromString(TEXT("Craft"));
}

void ACraftingTable::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor) return;

	InteractingPlayer = Interactor;
	Interactor->ToggleCrafting();
}

// --- Out-of-range close ---

void ACraftingTable::OnBoxEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (Player && Player == InteractingPlayer.Get())
	{
		Player->CloseCrafting();
		InteractingPlayer = nullptr;
	}
}
