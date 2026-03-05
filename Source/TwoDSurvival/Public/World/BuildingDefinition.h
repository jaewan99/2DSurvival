// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingDefinition.generated.h"

UENUM(BlueprintType)
enum class EBuildingType : uint8
{
	House     UMETA(DisplayName = "House"),
	Hospital  UMETA(DisplayName = "Hospital"),
	Store     UMETA(DisplayName = "Store"),
};

/**
 * Data asset describing a procedurally generated building interior.
 * Assign to UStreetDefinition::BuildingDefinition on any street definition where bIsPCGBuilding=true.
 *
 * ABuildingGenerator reads this at runtime and spawns room/stair/elevator actors in a grid.
 * Grid layout: rooms along the X axis, floors stacked along the Z axis.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UBuildingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	EBuildingType BuildingType = EBuildingType::House;

	// Number of floors in this building.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 FloorCount = 3;

	// Number of rooms per floor (excluding stairwell/elevator).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 RoomsPerFloor = 4;

	// Width of one room cell (cm). Also the X spacing between room origins.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float RoomWidth = 800.f;

	// Vertical distance between floor origins (cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float FloorHeight = 400.f;

	// Local X offset (from ABuildingGenerator origin) of the staircase column.
	// Must be a multiple of RoomWidth. e.g. RoomWidth * (RoomsPerFloor - 1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float StairX = 3200.f;

	// Whether this building has an elevator shaft.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator")
	bool bHasElevator = false;

	// Fixed local X position for the elevator room on every floor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator",
		meta = (EditCondition = "bHasElevator"))
	float ElevatorX = 200.f;

	// Blueprint actor class spawned for each room cell.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<AActor> RoomActorClass;

	// Blueprint actor class spawned at the staircase column on each floor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<AActor> StairsActorClass;

	// Blueprint actor class for the elevator room on each floor. Empty room for now.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors",
		meta = (EditCondition = "bHasElevator"))
	TSubclassOf<AActor> ElevatorRoomActorClass;
};
