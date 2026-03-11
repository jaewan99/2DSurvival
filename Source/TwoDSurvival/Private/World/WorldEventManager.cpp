// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WorldEventManager.h"
#include "World/TimeManager.h"
#include "World/NPCActor.h"
#include "World/WorldItem.h"
#include "Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

AWorldEventManager::AWorldEventManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWorldEventManager::BeginPlay()
{
	Super::BeginPlay();

	// Find the ATimeManager in the world and bind to day/night transitions.
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATimeManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		if (ATimeManager* TM = Cast<ATimeManager>(Managers[0]))
		{
			TimeManagerRef = TM;
			TM->OnDayPhaseChanged.AddDynamic(this, &AWorldEventManager::OnDayPhaseChanged);
			UE_LOG(LogTemp, Log, TEXT("[WorldEventManager] Bound to ATimeManager."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldEventManager] No ATimeManager found in the world — events will not fire."));
	}
}

// ── Day/night callback ────────────────────────────────────────────────────────

void AWorldEventManager::OnDayPhaseChanged(bool bIsNight)
{
	if (bIsNight)
	{
		RollEvents();
	}
	else
	{
		// Dawn — despawn any active travelling trader.
		if (ActiveTrader && IsValid(ActiveTrader))
		{
			ActiveTrader->Destroy();
			ActiveTrader = nullptr;
			UE_LOG(LogTemp, Log, TEXT("[WorldEventManager] Travelling trader despawned at dawn."));
		}
	}
}

// ── Event rolling ─────────────────────────────────────────────────────────────

void AWorldEventManager::RollEvents()
{
	ABaseCharacter* Player = GetPlayerCharacter();
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldEventManager] RollEvents: no player found, skipping."));
		return;
	}

	const int32 Today = GetCurrentDay();

	for (const FWorldEvent& Event : EventTable)
	{
		if (Event.EventID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldEventManager] Skipping event with empty EventID."));
			continue;
		}

		// Cooldown check.
		if (const int32* LastDay = LastFiredDay.Find(Event.EventID))
		{
			if ((Today - *LastDay) < Event.MinDaysBetween)
			{
				UE_LOG(LogTemp, Verbose,
					TEXT("[WorldEventManager] '%s' on cooldown (%d/%d days)."),
					*Event.EventID.ToString(), Today - *LastDay, Event.MinDaysBetween);
				continue;
			}
		}

		// Chance roll.
		if (FMath::FRand() > Event.SpawnChance) continue;

		// Mark fired.
		LastFiredDay.Add(Event.EventID, Today);

		// Execute the event.
		switch (Event.EventType)
		{
		case EWorldEventType::TravellingTrader:
			SpawnTravellingTrader(Event, Player);
			break;

		case EWorldEventType::ScavengerRaid:
			SpawnScavengerRaid(Event, Player);
			break;

		case EWorldEventType::SupplyCrate:
			SpawnSupplyCrate(Event, Player);
			break;
		}

		// Notify the HUD.
		if (!Event.BannerText.IsEmpty())
		{
			OnWorldEventStarted.Broadcast(Event.EventType, Event.BannerText);
		}
	}
}

int32 AWorldEventManager::GetCurrentDay() const
{
	if (TimeManagerRef.IsValid())
	{
		ATimeManager* TM = TimeManagerRef.Get();
		return (TM->CurrentYear - 1) * 12 * TM->DaysPerMonth
			 + (TM->CurrentMonth - 1) * TM->DaysPerMonth
			 + TM->CurrentDay;
	}
	return 0;
}

// ── Per-type spawn helpers ─────────────────────────────────────────────────────

void AWorldEventManager::SpawnTravellingTrader(const FWorldEvent& Event, ABaseCharacter* Player)
{
	if (!Event.TraderClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldEventManager] TravellingTrader '%s' has no TraderClass set — skipping."),
			*Event.EventID.ToString());
		return;
	}

	// Destroy any previous trader (shouldn't normally be active, but defensive).
	if (ActiveTrader && IsValid(ActiveTrader))
	{
		ActiveTrader->Destroy();
		ActiveTrader = nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLoc = GetScatterLocation(Player->GetActorLocation());
	ActiveTrader = GetWorld()->SpawnActor<ANPCActor>(Event.TraderClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (ActiveTrader)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[WorldEventManager] TravellingTrader '%s' spawned at X=%.0f."),
			*Event.EventID.ToString(), SpawnLoc.X);
	}
}

void AWorldEventManager::SpawnScavengerRaid(const FWorldEvent& Event, ABaseCharacter* Player)
{
	if (!Event.EnemyClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldEventManager] ScavengerRaid '%s' has no EnemyClass set — skipping."),
			*Event.EventID.ToString());
		return;
	}

	const int32 Count = FMath::RandRange(Event.MinEnemies, FMath::Max(Event.MinEnemies, Event.MaxEnemies));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 Spawned = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector SpawnLoc = GetScatterLocation(Player->GetActorLocation());
		if (GetWorld()->SpawnActor<ACharacter>(Event.EnemyClass, SpawnLoc, FRotator::ZeroRotator, Params))
		{
			++Spawned;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[WorldEventManager] ScavengerRaid '%s' — %d/%d enemies spawned."),
		*Event.EventID.ToString(), Spawned, Count);
}

void AWorldEventManager::SpawnSupplyCrate(const FWorldEvent& Event, ABaseCharacter* Player)
{
	if (Event.CrateLootTable.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldEventManager] SupplyCrate '%s' has empty CrateLootTable — skipping."),
			*Event.EventID.ToString());
		return;
	}

	const int32 Rolls = FMath::RandRange(
		Event.MinCrateRolls,
		FMath::Max(Event.MinCrateRolls, Event.MaxCrateRolls));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 Spawned = 0;
	for (int32 i = 0; i < Rolls; ++i)
	{
		// Each roll picks one random loot entry weighted by DropChance.
		for (const FLootEntry& Entry : Event.CrateLootTable)
		{
			if (!Entry.ItemDef) continue;
			if (FMath::FRand() > Entry.DropChance) continue;

			const int32 Qty = FMath::RandRange(Entry.MinCount, FMath::Max(Entry.MinCount, Entry.MaxCount));
			const FVector SpawnLoc = GetScatterLocation(Player->GetActorLocation());

			AWorldItem* Item = GetWorld()->SpawnActor<AWorldItem>(
				AWorldItem::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

			if (Item)
			{
				Item->ItemDef = Entry.ItemDef;
				Item->Quantity = Qty;
				++Spawned;
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[WorldEventManager] SupplyCrate '%s' — %d items spawned (%d rolls)."),
		*Event.EventID.ToString(), Spawned, Rolls);
}

// ── Utilities ────────────────────────────────────────────────────────────────

ABaseCharacter* AWorldEventManager::GetPlayerCharacter() const
{
	return Cast<ABaseCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

FVector AWorldEventManager::GetScatterLocation(const FVector& Origin) const
{
	// 2D side-scroller: scatter only on the X axis. Y and Z stay at the player's position
	// so spawned actors land on the same ground plane.
	const float OffsetX = FMath::FRandRange(-SpawnScatterRadius, SpawnScatterRadius);
	return FVector(Origin.X + OffsetX, Origin.Y, Origin.Z);
}
