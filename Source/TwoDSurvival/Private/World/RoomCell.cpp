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
	// Positioned at room origin; SetRoomDimensions centers it horizontally.

	// ── Ceiling ──────────────────────────────────────────────────────────────
	Ceiling = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ceiling"));
	Ceiling->SetupAttachment(RootComponent);
	Ceiling->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Left wall ────────────────────────────────────────────────────────────
	LeftWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWall"));
	LeftWall->SetupAttachment(RootComponent);
	LeftWall->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Right wall ───────────────────────────────────────────────────────────
	RightWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWall"));
	RightWall->SetupAttachment(RootComponent);
	RightWall->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Back wall ────────────────────────────────────────────────────────────
	// Decorative backdrop; no collision needed — the player can't reach it on the Y axis.
	BackWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWall"));
	BackWall->SetupAttachment(RootComponent);
	BackWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// ── ABuildingGenerator interface ──────────────────────────────────────────────

void ARoomCell::SetRoomDimensions(float Width, float Height)
{
	// All positions are relative to the room's bottom-left origin (actor location).
	// Meshes should be authored as unit-sized (100 cm) planes or matching geometry;
	// use the component's scale in the Blueprint viewport to fine-tune if needed.

	// Floor — bottom edge, centered horizontally
	Floor->SetRelativeLocation(FVector(Width * 0.5f, 0.f, 0.f));

	// Ceiling — top edge, centered horizontally
	Ceiling->SetRelativeLocation(FVector(Width * 0.5f, 0.f, Height));

	// Left wall — left edge, centered vertically
	LeftWall->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

	// Right wall — right edge, centered vertically
	RightWall->SetRelativeLocation(FVector(Width, 0.f, Height * 0.5f));

	// Back wall — centered in the room on both axes
	BackWall->SetRelativeLocation(FVector(Width * 0.5f, 0.f, Height * 0.5f));
}

// ── ApplyRoomDefinition ───────────────────────────────────────────────────────

void ARoomCell::ApplyRoomDefinition_Implementation(URoomDefinition* Definition)
{
	// Base is a no-op — override in the Blueprint child if archetype-specific logic is needed.
}
