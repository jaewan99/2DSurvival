// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/HealthTypes.h"
#include "World/WeatherManager.h"   // EWeatherState
#include "Components/JournalComponent.h"    // FJournalEntry
#include "Combat/StatusEffectTypes.h"       // FActiveStatusEffect
#include "TwoDSurvivalSaveGame.generated.h"

/**
 * One edge in the runtime street graph.
 * Stores FromStreetID → ExitID → ToStreetID so the graph can be rebuilt on load
 * without re-running the random generation.
 */
USTRUCT()
struct FSavedStreetConnection
{
	GENERATED_BODY()

	UPROPERTY()
	FName FromStreetID;

	UPROPERTY()
	FName ExitID;

	UPROPERTY()
	FName ToStreetID;
};

/** Serialized representation of one player-placed actor. */
USTRUCT()
struct FPlacedActorSaveData
{
	GENERATED_BODY()

	/** Stable GUID matching APlaceableActor::PlacementID — used to avoid duplicates on load. */
	UPROPERTY()
	FGuid PlacementID;

	/** Soft path to the Blueprint subclass so we can re-spawn the correct actor on load. */
	UPROPERTY()
	FSoftClassPath ActorClass;

	/** World transform at the time of save. */
	UPROPERTY()
	FTransform Transform;

	/** ItemID of the SourceItemDef — restored at load so pick-up returns the correct item. */
	UPROPERTY()
	FName ItemDefID;
};

/** Serialized representation of one inventory slot. */
USTRUCT()
struct FSavedInventorySlot
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;

	UPROPERTY()
	int32 Quantity = 0;
};

/** Serialized representation of one hotbar slot. */
USTRUCT()
struct FSavedHotbarSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;
};

/** Serialized representation of one body part's health. */
USTRUCT()
struct FSavedBodyPartHealth
{
	GENERATED_BODY()

	UPROPERTY()
	EBodyPart Part = EBodyPart::Body;

	UPROPERTY()
	float CurrentHealth = 0.f;

	UPROPERTY()
	float MaxHealth = 0.f;
};

/**
 * Save game object that persists player state:
 * inventory, hotbar, equipped weapon, body part health, and world position.
 */
UCLASS()
class TWODSURVIVAL_API UTwoDSurvivalSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSavedInventorySlot> InventorySlots;

	UPROPERTY()
	int32 BaseSlotCount = 0;

	UPROPERTY()
	TArray<FSavedHotbarSlot> HotbarSlots;

	UPROPERTY()
	int32 ActiveHotbarSlot = 0;

	UPROPERTY()
	FName EquippedWeaponItemID;

	UPROPERTY()
	TArray<FSavedBodyPartHealth> BodyPartHealthValues;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY()
	float SavedHunger = 100.f;

	UPROPERTY()
	float SavedThirst = 100.f;

	UPROPERTY()
	float SavedFatigue = 100.f;

	UPROPERTY()
	float SavedMood = 100.f;

	UPROPERTY()
	TSet<FName> VisitedStreetIDs;

	// NPCIDs for which the player has completed the trade offer.
	UPROPERTY()
	TSet<FName> CompletedNPCTrades;

	// --- Calendar ---
	UPROPERTY()
	int32 SavedDay = 1;

	UPROPERTY()
	int32 SavedMonth = 1;

	UPROPERTY()
	int32 SavedYear = 1;

	/** Normalized time of day (0–1) at the moment of save. */
	UPROPERTY()
	float SavedTimeOfDay = 0.25f;

	// --- Weather ---
	UPROPERTY()
	EWeatherState SavedWeatherState = EWeatherState::Clear;

	/** Elapsed seconds in the current weather state at the moment of save. */
	UPROPERTY()
	float SavedWeatherElapsed = 0.f;

	// --- Journal ---
	UPROPERTY()
	TArray<FJournalEntry> JournalEntries;

	// --- Status Effects ---
	UPROPERTY()
	TArray<FActiveStatusEffect> SavedStatusEffects;

	// --- Skills ---
	// Indexed by (int32)ESkillType: [0]=Combat, [1]=Crafting, [2]=Scavenging.
	UPROPERTY()
	TArray<int32> SavedSkillLevels;

	UPROPERTY()
	TArray<int32> SavedSkillXP;

	// --- Crafting ---
	// Recipe IDs the player has learned by reading books/schematics/magazines.
	UPROPERTY()
	TSet<FName> LearnedRecipeIDs;

	// --- Placed actors ---
	// All APlaceableActor instances the player has confirmed in the world.
	// Ghost actors (bIsGhost=true) are excluded.
	UPROPERTY()
	TArray<FPlacedActorSaveData> PlacedActors;

	// --- Street graph ---
	// Runtime-generated street connections (FromStreetID → ExitID → ToStreetID).
	// Only populated after the first GenerateCityGraph() call or on save.
	UPROPERTY()
	TArray<FSavedStreetConnection> SavedStreetGraph;

	/** True once the city graph has been generated and saved at least once. */
	UPROPERTY()
	bool bStreetGraphGenerated = false;
};
