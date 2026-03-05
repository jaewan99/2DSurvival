// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/StreetDefinition.h"
#include "StreetManager.generated.h"

class ULevelStreamingDynamic;

// Fired when the active street changes — UStreetHUDWidget binds this to refresh arrows.
DECLARE_MULTICAST_DELEGATE(FOnStreetChanged);

/**
 * Lives on the GameInstance — persists for the lifetime of the game session.
 *
 * Manages seamless level streaming for the street navigation system:
 *   - Loads the starting street at game start (called by AStreetBootstrapper)
 *   - When the player crosses a Left/Right exit, loads the adjacent street and unloads the old one
 *   - Up exits remain intentional interactive gates (handled separately)
 *
 * Streets are sublevels placed at world X offsets. Street A at X=0 with width 4000
 * means Street B (to the right) loads at X=4000.
 */
UCLASS()
class TWODSURVIVAL_API UStreetManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Called by AStreetBootstrapper in BeginPlay to load the initial street.
	void InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset = FVector::ZeroVector);

	// Called by AStreetExit when the player walks through a Left/Right boundary.
	// No-op if no exit is defined in that direction or a transition is already in progress.
	void OnPlayerCrossedExit(EExitDirection Direction);

	// Fired after a street transition completes (new street fully loaded and active).
	FOnStreetChanged OnStreetChanged;

	// The street the player is currently in.
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	TObjectPtr<UStreetDefinition> CurrentStreet;

	// World-space origin (X offset) of the currently loaded street.
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	FVector CurrentStreetWorldOffset = FVector::ZeroVector;

private:
	// The streaming level the player is currently inside.
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> ActiveStreaming;

	// Pending state set before LoadStreet — applied in OnNewStreetShown.
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> PendingStreaming;

	TObjectPtr<UStreetDefinition> PendingStreet;
	FVector PendingOffset = FVector::ZeroVector;

	// Previous streaming level queued for unload once the new one is shown.
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> StreamingToUnload;

	// Guards against overlapping transitions (e.g. player rapidly crossing back and forth).
	bool bTransitionInProgress = false;

	void LoadStreet(UStreetDefinition* Street, FVector WorldOffset);

	// Bound to ULevelStreamingDynamic::OnLevelShown — fires when the new street is fully loaded.
	UFUNCTION()
	void OnNewStreetShown();

	// Returns the world offset where the adjacent street should be placed.
	FVector ComputeAdjacentOffset(EExitDirection Direction, UStreetDefinition* AdjacentStreet) const;
};
