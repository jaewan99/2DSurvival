// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/CityDefinition.h"
#include "StreetDefinition.generated.h"

class UStreetDefinition;

// ── Exit layout ───────────────────────────────────────────────────────────────

/**
 * Controls how the destination sublevel is placed in the world when this exit fires.
 *
 *  AdjacentRight — loaded at CurrentOffset.X + CurrentStreet.StreetWidth  (walk-through, no teleport)
 *  AdjacentLeft  — loaded at CurrentOffset.X - NextStreet.StreetWidth      (walk-through, no teleport)
 *  Building      — loaded at BuildingWorldX (far off-grid); player is teleported to the
 *                  AExitSpawnPoint inside the destination level whose SpawnID matches this ExitID.
 */
UENUM(BlueprintType)
enum class EExitLayout : uint8
{
	AdjacentRight UMETA(DisplayName = "Adjacent Right"),
	AdjacentLeft  UMETA(DisplayName = "Adjacent Left"),
	Building      UMETA(DisplayName = "Building"),
};

// ── Exit link struct ─────────────────────────────────────────────────────────

/**
 * One named exit on a UStreetDefinition.
 *
 * Place an AStreetExit trigger (walk-through) or ABuildingEntrance (press-E) in the
 * sublevel with a matching ExitID to activate this link.
 */
USTRUCT(BlueprintType)
struct FStreetExitLink
{
	GENERATED_BODY()

	/**
	 * Unique name for this exit on the current street (e.g. "Left", "Right", "FrontDoor", "AlleyGate").
	 * Must match AStreetExit::ExitID (or ABuildingEntrance::BuildingExitID) in the sublevel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ExitID;

	/** Where this exit leads. Null = blocked (trigger ignored, no HUD arrow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStreetDefinition> Destination;

	/**
	 * How the destination level is positioned in the world.
	 * AdjacentRight/Left: levels are placed side-by-side — player walks across naturally.
	 * Building: destination is loaded at a fixed far-off X; player is teleported to a spawn point.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EExitLayout Layout = EExitLayout::AdjacentRight;
};

// ── Street definition ─────────────────────────────────────────────────────────

/**
 * Data asset describing one street or building interior: its sublevel, width, and exit connections.
 *
 * Create one DA_Street_* per playable location. Wire up Exits in the Details panel —
 * there is no longer a fixed Left/Right/Up limit. Name exits freely (e.g. "FrontDoor",
 * "BackAlley", "RoofHatch") and place matching AStreetExit triggers in the sublevel.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UStreetDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique stable ID — never rename after first use (used for save data keying). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FName StreetID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FText StreetName;

	/** Width of this street in world units (cm). Used by AdjacentRight/Left offset calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	float StreetWidth = 4000.f;

	/** The sublevel asset to stream in for this street or building interior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	TSoftObjectPtr<UWorld> Level;

	/**
	 * All exits on this street. Add as many as needed.
	 * Each entry's ExitID must match an AStreetExit::ExitID (or ABuildingEntrance::BuildingExitID)
	 * placed inside this street's sublevel.
	 *
	 * Convention: name exits "Left" and "Right" for the StreetHUD arrows to display automatically.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exits")
	TArray<FStreetExitLink> Exits;

	/** True when this definition represents a building interior, not an open street. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	bool bIsPCGBuilding = false;

	/** The city this street belongs to. Null = highway segment or wilderness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City")
	TObjectPtr<UCityDefinition> OwnerCity;

	/** True for highway segments connecting two cities. Shown with amber lines on the map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City")
	bool bIsHighway = false;

	/** Short label for the world map node. Falls back to StreetName if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText MapLabel;

	/** Returns the exit link for the given ID, or nullptr if not found. */
	const FStreetExitLink* GetExit(FName ExitID) const;
};
