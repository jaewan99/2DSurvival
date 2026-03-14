// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/RoomDefinition.h"
#include "BuildingDefinition.generated.h"

class URoomDefinition;
class ARoomCell;

UENUM(BlueprintType)
enum class EBuildingType : uint8
{
	House       UMETA(DisplayName = "House"),
	Hospital    UMETA(DisplayName = "Hospital"),
	Store       UMETA(DisplayName = "Store"),
	Restaurant  UMETA(DisplayName = "Restaurant"),
};

// ── Floor layout ──────────────────────────────────────────────────────────────

/**
 * Defines the category requirement for each room slot on one floor, left to right.
 *
 * The generator fills each slot by picking a random URoomDefinition from the
 * building's RoomPool whose Category matches the slot requirement.
 *
 * If SlotCategories has fewer entries than RoomsPerFloor, the last entry repeats.
 * Use ERoomCategory::Any for a slot with no constraint.
 *
 * House example:
 *   Ground floor: [Social, Social, Utility]   → Kitchen/LivingRoom + Bathroom
 *   Upper floors: [Private, Private, Utility]  → Bedrooms + Bathroom
 */
USTRUCT(BlueprintType)
struct TWODSURVIVAL_API FFloorLayout
{
	GENERATED_BODY()

	// Category requirement per slot, ordered left → right.
	// Slot index matches room index in the generator grid.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TArray<ERoomCategory> SlotCategories;
};

// ── Building definition ───────────────────────────────────────────────────────

/**
 * Data asset describing a procedurally generated building interior.
 *
 * Set up RoomPool with all room archetypes this building can use, then define
 * FloorLayouts — one entry per floor (index 0 = ground floor). If FloorLayouts
 * has fewer entries than FloorCount, the last entry repeats for remaining floors.
 *
 * Example house setup:
 *   RoomPool:     [DA_Room_Kitchen, DA_Room_LivingRoom, DA_Room_Bedroom, DA_Room_Bathroom]
 *   FloorLayouts:
 *     [0] SlotCategories: [Social, Social, Utility]    ← ground floor
 *     [1] SlotCategories: [Private, Private, Utility]  ← upper floors (repeats)
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UBuildingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	EBuildingType BuildingType = EBuildingType::House;

	// ── Layout ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 FloorCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 RoomsPerFloor = 4;

	// Width of one room cell (cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float RoomWidth = 800.f;

	// Vertical distance between floor origins (cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	float FloorHeight = 400.f;

	// ── Stairs ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	bool bRandomizeStairs = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout",
		meta = (EditCondition = "bRandomizeStairs", ClampMin = "1"))
	int32 MinStairsPerFloor = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout",
		meta = (EditCondition = "bRandomizeStairs", ClampMin = "1"))
	int32 MaxStairsPerFloor = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout",
		meta = (EditCondition = "bRandomizeStairs"))
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout",
		meta = (EditCondition = "!bRandomizeStairs"))
	float StairX = 3200.f;

	// ── Elevator ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator")
	bool bHasElevator = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elevator",
		meta = (EditCondition = "bHasElevator"))
	float ElevatorX = 200.f;

	// ── Room pool & floor layouts ─────────────────────────────────────────────

	/**
	 * All room archetypes this building can use.
	 * The generator filters this list by each slot's ERoomCategory requirement.
	 * Add every DA_Room_* asset relevant to this building type here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	TArray<TObjectPtr<URoomDefinition>> RoomPool;

	/**
	 * Category sequence per floor (index 0 = ground floor).
	 * If FloorLayouts.Num() < FloorCount, the last entry repeats for remaining floors.
	 * Each FFloorLayout defines one category per room slot, left to right.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rooms")
	TArray<FFloorLayout> FloorLayouts;

	// ── Actors ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<AActor> StairsActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors",
		meta = (EditCondition = "bHasElevator"))
	TSubclassOf<AActor> ElevatorRoomActorClass;

	/**
	 * Room cell spawned at ground floor, leftmost slot (slot 0).
	 * Should have a door/entrance on its left wall facing the street.
	 * If null, the slot falls back to the RoomPool.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<ARoomCell> LeftEntranceActorClass;

	/**
	 * Room cell spawned at ground floor, rightmost slot (slot RoomsPerFloor-1).
	 * Should have a door/entrance on its right wall facing the street.
	 * If null, the slot falls back to the RoomPool.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<ARoomCell> RightEntranceActorClass;
};
