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

// ── Room definition ───────────────────────────────────────────────────────────

/**
 * Defines one room archetype: category, Blueprint to spawn, and materials.
 *
 * All props, lootables, and doors live inside the RoomActorClass Blueprint
 * as Child Actor Components — the generator does not spawn them separately.
 *
 * Create one asset per archetype (e.g. DA_Room_Bedroom, DA_Room_Kitchen).
 * Add all relevant assets to UBuildingDefinition::RoomPool.
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

};
