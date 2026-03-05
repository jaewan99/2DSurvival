// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/StreetDefinition.h"
#include "StreetManager.generated.h"

class ULevelStreamingDynamic;

// Fired when the active street/building changes — UStreetHUDWidget binds this to refresh arrows.
DECLARE_MULTICAST_DELEGATE(FOnStreetChanged);

/**
 * Lives on the GameInstance — persists for the lifetime of the game session.
 *
 * Manages seamless level streaming for street navigation and building entry/exit:
 *   - Loads the starting street at game start (called by AStreetBootstrapper)
 *   - Left/Right exits: loads the adjacent street, unloads the old one
 *   - Up exit (building entrance): saves return state, loads the building sublevel at
 *     X=100000 (far from the street grid), injects PCG parameters, teleports the player
 *     inside. Pressing Up again (ABuildingEntrance inside building) exits back to the street.
 *
 * Streets are placed at world X offsets (Street A at X=0, Street B at X=StreetA.Width, …).
 * Buildings are always placed at X=BuildingWorldX to avoid overlapping the street grid.
 */
UCLASS()
class TWODSURVIVAL_API UStreetManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Called by AStreetBootstrapper in BeginPlay to load the initial street.
	void InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset = FVector::ZeroVector);

	// Called by AStreetExit (Left/Right) or ABuildingEntrance (Up).
	// Up toggles between entering and exiting a building based on bIsInsideBuilding.
	void OnPlayerCrossedExit(EExitDirection Direction);

	// Fired after any street/building transition completes.
	FOnStreetChanged OnStreetChanged;

	// The street (or building definition) the player is currently in.
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	TObjectPtr<UStreetDefinition> CurrentStreet;

	// World-space origin of the currently active sublevel.
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	FVector CurrentStreetWorldOffset = FVector::ZeroVector;

	// True while the player is inside a building sublevel (not on a street).
	// ABuildingEntrance reads this to choose the correct interaction prompt.
	UPROPERTY(BlueprintReadOnly, Category = "Building")
	bool bIsInsideBuilding = false;

private:
	// Building sublevels are placed at this fixed X offset, clear of the street grid.
	static constexpr float BuildingWorldX = 100000.f;

	// ── Streaming handles ─────────────────────────────────────────────────────
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> ActiveStreaming;

	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> PendingStreaming;

	TObjectPtr<UStreetDefinition> PendingStreet;
	FVector PendingOffset = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> StreamingToUnload;

	bool bTransitionInProgress = false;

	// ── Building return state ──────────────────────────────────────────────────
	// Saved when the player enters a building; restored on exit.
	TObjectPtr<UStreetDefinition> ReturnStreet;
	FVector ReturnStreetOffset    = FVector::ZeroVector;
	FVector ReturnPlayerLocation  = FVector::ZeroVector;

	// ── Transition type ────────────────────────────────────────────────────────
	// Determines what OnNewStreetShown does after the load completes.
	enum class ETransitionType : uint8 { Street, EnterBuilding, ExitBuilding };
	ETransitionType PendingTransitionType = ETransitionType::Street;

	// ── Internal helpers ───────────────────────────────────────────────────────
	void LoadStreet(UStreetDefinition* Street, FVector WorldOffset);

	// Bound to ULevelStreamingDynamic::OnLevelShown — fires once per load.
	UFUNCTION()
	void OnNewStreetShown();

	// Computes where to place an adjacent Left/Right street in world space.
	FVector ComputeAdjacentOffset(EExitDirection Direction, UStreetDefinition* AdjacentStreet) const;

	// Finds the ABuildingGenerator in the loaded level and calls Generate() on it.
	void TriggerBuildingGeneration(ULevel* Level);

	// Teleports the player pawn to the first ABuildingEntrance found in Level.
	void TeleportPlayerToBuildingEntrance(ULevel* Level);

	// Teleports the player pawn to an explicit world location.
	void TeleportPlayerToLocation(FVector Location);

	// Returns the player pawn's current world location (ZeroVector if unavailable).
	FVector GetPlayerLocation() const;
};
