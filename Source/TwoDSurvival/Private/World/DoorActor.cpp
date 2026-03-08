// Fill out your copyright notice in the Description page of Project Settings.

#include "World/DoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

ADoorActor::ADoorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Interaction zone — same width/depth as a standard room boundary
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(80.f, 80.f, 120.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();
	bIsOpen = bStartOpen;
	ApplyOpenState();
}

// ── IInteractable ─────────────────────────────────────────────────────────────

EInteractionType ADoorActor::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float ADoorActor::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText ADoorActor::GetInteractionPrompt_Implementation()
{
	if (bIsLocked) return FText::FromString(TEXT("Locked"));
	return FText::FromString(bIsOpen ? TEXT("Close Door") : TEXT("Open Door"));
}

void ADoorActor::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (bIsLocked) return;
	bIsOpen = !bIsOpen;
	ApplyOpenState();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void ADoorActor::ApplyOpenState()
{
	DoorMesh->SetVisibility(!bIsOpen);
	DoorMesh->SetCollisionEnabled(bIsOpen ? ECollisionEnabled::NoCollision
	                                       : ECollisionEnabled::QueryAndPhysics);
}
