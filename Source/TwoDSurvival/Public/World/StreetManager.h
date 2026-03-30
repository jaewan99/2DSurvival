// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/StreetDefinition.h"
#include "StreetManager.generated.h"

class ULevelStreamingDynamic;
class UTwoDSurvivalSaveGame;

// Fired when the active street/building changes — UStreetHUDWidget binds this to refresh arrows.
DECLARE_MULTICAST_DELEGATE(FOnStreetChanged);

// Fired when the player moves into a different city (or onto a highway / back into a city).
DECLARE_MULTICAST_DELEGATE(FOnCityChanged);

/**
 * Lives on the GameInstance — persists for the lifetime of the game session.
 *
 * Manages seamless level streaming for street navigation and building entry/exit.
 *
 * Exits are now named (FName ExitID) rather than fixed Left/Right/Up.
 * Each UStreetDefinition has a TArray<FStreetExitLink> Exits; each link specifies
 * where it leads and how the destination level is positioned (AdjacentRight/Left/Building).
 *
 * Walk-through exits (AdjacentRight/Left): destination level is placed next to the current one;
 *   player walks across naturally — no teleport.
 * Building exits: destination is loaded at a far-off X (BuildingWorldX); player is teleported
 *   to an AExitSpawnPoint whose SpawnID matches the ExitID used to enter.
 */
UCLASS()
class TWODSURVIVAL_API UStreetManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Called by AStreetBootstrapper in BeginPlay to load the initial street. */
	void InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset = FVector::ZeroVector);

	/**
	 * Called by AStreetExit (walk-through) or ABuildingEntrance (press-E, street side).
	 * ExitID must match a FStreetExitLink::ExitID on CurrentStreet.
	 */
	void OnPlayerCrossedExit(FName ExitID);

	/**
	 * Called by ABuildingEntrance (press-E, building side).
	 * Exits the current building and restores the street the player came from.
	 */
	void OnPlayerExitBuilding();

	/** Fired after any street/building transition completes. */
	FOnStreetChanged OnStreetChanged;

	/** Fired when the player enters or leaves a city (including going onto a highway). */
	FOnCityChanged OnCityChanged;

	/** The city the player is currently in. Null while on a highway or wilderness street. */
	UPROPERTY(BlueprintReadOnly, Category = "City")
	TObjectPtr<UCityDefinition> CurrentCity;

	/** Streets the player has visited this session (+ restored from save). */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TSet<FName> VisitedStreetIDs;

	const TSet<FName>& GetVisitedStreets() const { return VisitedStreetIDs; }

	/** Called by LoadGame to restore visited streets from save data. */
	void RestoreVisitedStreets(const TSet<FName>& Saved) { VisitedStreetIDs = Saved; }

	/**
	 * Resolves the destination street for a given exit on the given street.
	 * First checks the exit's hard-wired Destination; if null, checks the runtime
	 * generated graph. Returns null if the exit is blocked or unknown.
	 */
	UStreetDefinition* ResolveExitDestination(UStreetDefinition* Street, FName ExitID) const;

	/**
	 * Returns true if the current street has a resolvable (non-blocked) exit with this ID.
	 * Used by StreetHUDWidget to decide whether to show directional arrows.
	 */
	bool HasResolvableExit(FName ExitID) const;

	/** Looks up a UStreetDefinition by its StreetID (from the scanned asset map). */
	UStreetDefinition* FindStreetByID(FName StreetID) const;

	/** Serialize the current runtime graph into a save game object. */
	void SaveGraphToSaveGame(UTwoDSurvivalSaveGame* Save) const;

	/** Restore the runtime graph from a save game object (skips generation). */
	void RestoreGraphFromSaveGame(const UTwoDSurvivalSaveGame* Save);

	/** The street the session started on (used as BFS root for map layout). */
	UFUNCTION(BlueprintCallable, Category = "Map")
	UStreetDefinition* GetStartingStreet() const { return StartingStreetDef; }

	/** The street or building definition the player is currently in. */
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	TObjectPtr<UStreetDefinition> CurrentStreet;

	/** World-space origin of the currently active sublevel. */
	UPROPERTY(BlueprintReadOnly, Category = "Street")
	FVector CurrentStreetWorldOffset = FVector::ZeroVector;

	/** True while the player is inside a building sublevel (not on a street). */
	UPROPERTY(BlueprintReadOnly, Category = "Building")
	bool bIsInsideBuilding = false;

private:
	// Building sublevels are placed at this fixed X offset, clear of the street grid.
	static constexpr float BuildingWorldX = 100000.f;

	UPROPERTY()
	TObjectPtr<UStreetDefinition> StartingStreetDef;

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
	TObjectPtr<UStreetDefinition> ReturnStreet;
	FVector ReturnStreetOffset   = FVector::ZeroVector;
	FVector ReturnPlayerLocation = FVector::ZeroVector;

	// The ExitID used to enter the building — matched against AExitSpawnPoint::SpawnID on load.
	FName PendingIncomingExitID = NAME_None;

	// ── Transition type ────────────────────────────────────────────────────────
	enum class ETransitionType : uint8 { Street, EnterBuilding, ExitBuilding };
	ETransitionType PendingTransitionType = ETransitionType::Street;

	// ── Runtime city graph ─────────────────────────────────────────────────────
	// Generated once per new game, then saved/restored. Maps StreetID → (ExitID → ToStreetID).
	TMap<FName, TMap<FName, FName>> GeneratedGraph;

	// All scanned UStreetDefinition assets, keyed by StreetID.
	UPROPERTY()
	TMap<FName, TObjectPtr<UStreetDefinition>> AllStreetDefsMap;

	bool bCityGenerated = false;

	/** Scan all UStreetDefinition assets into AllStreetDefsMap via AssetRegistry. */
	void ScanAllStreetDefs();

	/**
	 * Scan all city street defs via AssetRegistry, shuffle each city's pool,
	 * pick MinStreets–MaxStreets streets per city, and wire Left/Right connections.
	 * Called once on first game start (no saved graph).
	 */
	void GenerateCityGraph();

	// ── Internal helpers ───────────────────────────────────────────────────────
	void LoadStreet(UStreetDefinition* Street, FVector WorldOffset);

	// Bound to ULevelStreamingDynamic::OnLevelShown — fires once per load.
	UFUNCTION()
	void OnNewStreetShown();

	// Computes where to place an adjacent Left/Right street in world space.
	FVector ComputeAdjacentOffset(EExitLayout Layout, UStreetDefinition* AdjacentStreet) const;

	/**
	 * Scans Level for an AExitSpawnPoint whose SpawnID matches IncomingExitID and teleports the player there.
	 * Falls back to ABuildingEntrance if no matching spawn point is found (backward compatibility).
	 */
	void TeleportPlayerToSpawnPoint(ULevel* Level, FName IncomingExitID);

	// Teleports the player pawn to an explicit world location.
	void TeleportPlayerToLocation(FVector Location);

	// Returns the player pawn's current world location (ZeroVector if unavailable).
	FVector GetPlayerLocation() const;
};
