// Fill out your copyright notice in the Description page of Project Settings.

#include "World/RoomCell.h"
#include "World/RoomDefinition.h"
#include "Components/StaticMeshComponent.h"

ARoomCell::ARoomCell()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Floor — also serves as the ceiling for the room below.
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(RootComponent);
	Floor->SetCollisionProfileName(TEXT("BlockAll"));
}

// ── ABuildingGenerator interface ──────────────────────────────────────────────

void ARoomCell::SetRoomDimensions(float Width)
{
	Floor->SetRelativeLocation(FVector(Width * 0.5f, 0.f, 0.f));
}

void ARoomCell::ApplyRoomDefinition_Implementation(URoomDefinition* Definition)
{
	// Base is a no-op — override in the Blueprint child for archetype-specific logic.
}
