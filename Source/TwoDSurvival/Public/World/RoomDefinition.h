// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomDefinition.generated.h"

class ARoomCell;

// ── Room archetype label ──────────────────────────────────────────────────────

/** Descriptive label — editor only, does not affect runtime logic. */
UENUM(BlueprintType)
enum class ERoomArchetype : uint8
{
	Generic       UMETA(DisplayName = "Generic"),
	// House
	Bedroom       UMETA(DisplayName = "Bedroom"),
	Bathroom      UMETA(DisplayName = "Bathroom"),
	Kitchen       UMETA(DisplayName = "Kitchen"),
	LivingRoom    UMETA(DisplayName = "Living Room"),
	// Hospital
	Reception     UMETA(DisplayName = "Reception"),
	WaitingRoom   UMETA(DisplayName = "Waiting Room"),
	PatientWard   UMETA(DisplayName = "Patient Ward"),
	OperatingRoom UMETA(DisplayName = "Operating Room"),
	// Store
	ShopFloor     UMETA(DisplayName = "Shop Floor"),
	Counter       UMETA(DisplayName = "Counter"),
	StoreRoom     UMETA(DisplayName = "Store Room"),
	StaffRoom     UMETA(DisplayName = "Staff Room"),
	// Restaurant
	DiningArea    UMETA(DisplayName = "Dining Area"),
	Bar           UMETA(DisplayName = "Bar"),
};

// ── Room category — drives slot constraints ───────────────────────────────────

/**
 * Broad category used by the floor layout slot system.
 * Each slot in a FFloorLayout requires a category; the generator picks
 * a random URoomDefinition from the building's RoomPool that matches.
 *
 *  Social  — shared living spaces (Kitchen, LivingRoom, DiningArea, Reception, WaitingRoom)
 *  Private — individual-use rooms (Bedroom, PatientWard, OperatingRoom)
 *  Utility — support rooms      (Bathroom, StoreRoom, StaffRoom, Counter, Bar)
 *  Any     — no constraint, picks freely from the full RoomPool
 */
UENUM(BlueprintType)
enum class ERoomCategory : uint8
{
	Any     UMETA(DisplayName = "Any"),
	Social  UMETA(DisplayName = "Social"),
	Private UMETA(DisplayName = "Private"),
	Utility UMETA(DisplayName = "Utility"),
};

// ── Prop spawn entry ─────────────────────────────────────────────────────────

/**
 * One entry in a room's prop spawn table.
 *
 * The generator spawns ActorClass at (RoomOrigin + RelativeOffset) after placing
 * the room cell. Use this to populate rooms with enemies, NPCs, containers,
 * crafting tables, world props, etc. — no Blueprint changes needed per variant.
 *
 * RelativeOffset is in room-local space: X = along room width (left→right),
 * Z = height above floor origin. Y is ignored (generator keeps everything on
 * the street Y plane).
 */
USTRUCT(BlueprintType)
struct FRoomPropEntry
{
	GENERATED_BODY()

	/** Actor class to spawn (BP_EnemyBase, BP_NPCActor, BP_WoodPlanks, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> ActorClass;

	/** Offset from the room origin in local space (X along width, Z height). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector RelativeOffset = FVector::ZeroVector;

	/** 0–1 probability this entry spawns. 1 = always, 0.5 = 50% chance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 1.f;
};

// ── Room definition ───────────────────────────────────────────────────────────

/**
 * Defines one room variant: category, Blueprint geometry, and prop contents.
 *
 * The room Blueprint (RoomActorClass) contains only geometry — walls, floor,
 * ceiling, and facade. All variable content (enemies, NPCs, furniture, loot)
 * is listed in PropSpawnTable so the same geometry Blueprint can be reused
 * across many variants without duplicating Blueprints.
 *
 * Create one asset per room variant (e.g. DA_Room_Bedroom_Empty,
 * DA_Room_Bedroom_WithEnemy). Add all relevant assets to UBuildingDefinition::RoomPool.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API URoomDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Descriptive label — editor only.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	ERoomArchetype Archetype = ERoomArchetype::Generic;

	// Broad category — must match the slot requirement in FFloorLayout.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	ERoomCategory Category = ERoomCategory::Any;

	// Blueprint actor class to spawn. Must inherit from ARoomCell.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	TSubclassOf<ARoomCell> RoomActorClass;

	/**
	 * Actors spawned inside this room after the room cell is placed.
	 * Each entry specifies an actor class, a room-local offset, and a spawn chance.
	 * Use this to add enemies, NPCs, containers, crafting tables, props, etc.
	 * without baking them into the room Blueprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Props")
	TArray<FRoomPropEntry> PropSpawnTable;
};
