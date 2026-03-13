// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/EnemyTypes.h"
#include "WorldEventManager.generated.h"

class ANPCActor;
class ABaseCharacter;
class ATimeManager;
class AWorldItem;
class AWorldEventSpawnPoint;

// ── Event type ──────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EWorldEventType : uint8
{
	TravellingTrader  UMETA(DisplayName = "Travelling Trader"),
	ScavengerRaid     UMETA(DisplayName = "Scavenger Raid"),
	SupplyCrate       UMETA(DisplayName = "Supply Crate"),
};

// ── Delegate ────────────────────────────────────────────────────────────────

/** Broadcast when an event fires. BannerText is shown in UStreetHUDWidget. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldEventStarted, EWorldEventType, EventType, FText, BannerText);

// ── Data struct ─────────────────────────────────────────────────────────────

/**
 * One entry in the world event table.
 * Assign per-type fields based on the EventType you select.
 */
USTRUCT(BlueprintType)
struct TWODSURVIVAL_API FWorldEvent
{
	GENERATED_BODY()

	/** Stable identifier — used for cooldown tracking. Never rename after first use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FName EventID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	EWorldEventType EventType = EWorldEventType::SupplyCrate;

	/** Probability this event fires on any given night (0 = never, 1 = guaranteed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 0.3f;

	/** In-game days that must pass before this event can fire again. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event", meta = (ClampMin = "0"))
	int32 MinDaysBetween = 3;

	/** Text shown in the StreetHUD banner when this event fires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FText BannerText;

	// ── Travelling Trader ────────────────────────────────────────────────

	/**
	 * Blueprint child of ANPCActor to spawn for this trader.
	 * Assign a BP_NPC_Trader with NPCDef (trade offer) configured.
	 * Used only when EventType = TravellingTrader.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Trader")
	TSubclassOf<ANPCActor> TraderClass;

	// ── Scavenger Raid ───────────────────────────────────────────────────

	/**
	 * Enemy Blueprint class to spawn (e.g. BP_EnemyBase).
	 * Used only when EventType = ScavengerRaid.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Raid")
	TSubclassOf<ACharacter> EnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Raid", meta = (ClampMin = "1"))
	int32 MinEnemies = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Raid", meta = (ClampMin = "1"))
	int32 MaxEnemies = 4;

	// ── Supply Crate ─────────────────────────────────────────────────────

	/**
	 * Items eligible to appear in the crate. Same FLootEntry format as enemy drops.
	 * Used only when EventType = SupplyCrate.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Crate")
	TArray<FLootEntry> CrateLootTable;

	/** Min number of individual loot rolls per crate spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Crate", meta = (ClampMin = "1"))
	int32 MinCrateRolls = 3;

	/** Max number of individual loot rolls per crate spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event|Crate", meta = (ClampMin = "1"))
	int32 MaxCrateRolls = 5;
};

// ── Manager actor ────────────────────────────────────────────────────────────

/**
 * World-level manager for scheduled random events fired at the start of each night.
 * Place one BP_WorldEventManager in the persistent level.
 *
 * On each night start:
 *   - Iterates EventTable — skips entries whose MinDaysBetween cooldown has not elapsed.
 *   - Rolls SpawnChance for each eligible entry.
 *   - Fires the event: spawns trader NPC / raid enemies / supply crate items near the player.
 *   - Broadcasts OnWorldEventStarted → UStreetHUDWidget shows the BannerText for 5 s.
 *
 * Blueprint steps:
 *   1. Create Blueprint child BP_WorldEventManager (parent: AWorldEventManager).
 *      Place one instance in the persistent level.
 *   2. Fill EventTable in the Details panel. One entry per event type.
 *      Set EventID (unique, stable FName), EventType, SpawnChance, MinDaysBetween, BannerText.
 *   3. TravellingTrader entry → set TraderClass = BP_NPC_Trader
 *      (ANPCActor child with NPCDef trade offer set up).
 *      Trader despawns automatically at next dawn.
 *   4. ScavengerRaid entry → set EnemyClass = BP_EnemyBase (or any ACharacter subclass),
 *      MinEnemies / MaxEnemies.
 *   5. SupplyCrate entry → fill CrateLootTable, set MinCrateRolls / MaxCrateRolls.
 *   6. WBP_StreetHUD → add a TextBlock named EventBanner (top-center, collapsed by default).
 *      The banner text is set and shown automatically when an event fires.
 */
UCLASS()
class TWODSURVIVAL_API AWorldEventManager : public AActor
{
	GENERATED_BODY()

public:
	AWorldEventManager();

	/** Configurable event pool. One entry per event variant. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	TArray<FWorldEvent> EventTable;

	/**
	 * Fired when an event starts.
	 * UStreetHUDWidget binds this in NativeConstruct to display the notification banner.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWorldEventStarted OnWorldEventStarted;

	/** How many seconds the HUD notification banner stays visible after an event fires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	float BannerDuration = 5.f;

	/** If true, fires RollEvents() once at game start (after a short delay for the street to stream in).
	 *  Useful for populating the starting street with enemies/traders on load. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	bool bSpawnOnStart = false;

	/** Delay in seconds before the startup spawn fires. Increase if your street takes longer to stream in. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events",
		meta = (EditCondition = "bSpawnOnStart", ClampMin = "0.0"))
	float SpawnOnStartDelay = 1.5f;

	/**
	 * Fallback scatter radius (cm) on the X axis around the player.
	 * Only used when no AWorldEventSpawnPoint actors are placed in the level.
	 * If spawn points exist, this is ignored — locations come from those actors instead.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	float SpawnScatterRadius = 400.f;

protected:
	virtual void BeginPlay() override;

private:
	// Day index on which each EventID last fired. Used for MinDaysBetween cooldown.
	TMap<FName, int32> LastFiredDay;

	TWeakObjectPtr<ATimeManager> TimeManagerRef;

	// Active trader actor — destroyed automatically at the next dawn.
	UPROPERTY()
	TObjectPtr<ANPCActor> ActiveTrader;

	// All AWorldEventSpawnPoint actors found in the level at BeginPlay.
	UPROPERTY()
	TArray<TObjectPtr<AWorldEventSpawnPoint>> SpawnPoints;

	// Called each dawn/dusk by ATimeManager::OnDayPhaseChanged.
	UFUNCTION()
	void OnDayPhaseChanged(bool bIsNight);

	// Iterates EventTable and fires eligible events. Called at night and optionally on start.
	UFUNCTION()
	void RollEvents();

	// Absolute day count across all years/months — used for cooldown arithmetic.
	int32 GetCurrentDay() const;

	// Per-type spawn helpers.
	void SpawnTravellingTrader(const FWorldEvent& Event, ABaseCharacter* Player);
	void SpawnScavengerRaid(const FWorldEvent& Event, ABaseCharacter* Player);
	void SpawnSupplyCrate(const FWorldEvent& Event, ABaseCharacter* Player);

	// Finds and returns the local player character, or nullptr if not found.
	ABaseCharacter* GetPlayerCharacter() const;

	/**
	 * Returns a spawn location.
	 * If AWorldEventSpawnPoint actors exist in the level, picks one at random.
	 * Falls back to scattering on the X axis around Origin if none are placed.
	 */
	FVector GetSpawnLocation(const FVector& Origin) const;
};
