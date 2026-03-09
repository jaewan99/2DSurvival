// Fill out your copyright notice in the Description page of Project Settings.

#include "World/RoomCell.h"
#include "World/RoomDefinition.h"
#include "Components/StaticMeshComponent.h"

ARoomCell::ARoomCell()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scene root — generator places this at the room's bottom-left origin.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// ── Floor ────────────────────────────────────────────────────────────────
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(RootComponent);
	Floor->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Ceiling ──────────────────────────────────────────────────────────────
	Ceiling = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ceiling"));
	Ceiling->SetupAttachment(RootComponent);
	Ceiling->SetCollisionProfileName(TEXT("BlockAll"));

	// Walls are handled entirely in the Blueprint child viewport.
}

// ── ABuildingGenerator interface ──────────────────────────────────────────────

void ARoomCell::SetRoomDimensions(float Width, float Height)
{
	// Floor — bottom edge, centered horizontally
	Floor->SetRelativeLocation(FVector(Width * 0.5f, 0.f, 0.f));

	// Ceiling — top edge, centered horizontally
	Ceiling->SetRelativeLocation(FVector(Width * 0.5f, 0.f, Height));
}

// ── ApplyRoomDefinition ───────────────────────────────────────────────────────

void ARoomCell::ApplyRoomDefinition_Implementation(URoomDefinition* Definition)
{
	// Base is a no-op — override in the Blueprint child if archetype-specific logic is needed.
}
