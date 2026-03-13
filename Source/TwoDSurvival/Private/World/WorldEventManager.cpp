// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WorldEventManager.h"
#include "World/WorldEventSpawnPoint.h"
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

	if (bSpawnOnStart)
	{
		FTimerHandle StartupTimer;
		GetWorldTimerManager().SetTimer(StartupTimer, this, &AWorldEventManager::RollEvents, SpawnOnStartDelay, false);
		UE_LOG(LogTemp, Log, TEXT("[WorldEventManager] bSpawnOnStart=true — will fire in %.1fs."), SpawnOnStartDelay);
	}

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

	// Rescan spawn points now so we pick up actors from whichever sublevel is currently loaded.
	SpawnPoints.Reset();
	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldEventSpawnPoint::StaticClass(), FoundPoints);
	for (AActor* A : FoundPoints)
	{
		if (AWorldEventSpawnPoint* SP = Cast<AWorldEventSpawnPoint>(A))
		{
			SpawnPoints.Add(SP);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[WorldEventManager] %d spawn point(s) available for this night."), SpawnPoints.Num());
	GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan,
		FString::Printf(TEXT("[WorldEventManager] Night — %d spawn point(s) found."), SpawnPoints.Num()));

	const int32 Today = GetCurrentDay();

	for (const FWorldEvent& Event : EventTable)
	{
		if (Event.EventID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldEventManager] Skipping event with empty EventID."));
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, TEXT("[WorldEventManager] Skipped entry: EventID is empty."));
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
				GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
					FString::Printf(TEXT("[WorldEventManager] '%s' on cooldown (%d/%d days)."),
						*Event.EventID.ToString(), Today - *LastDay, Event.MinDaysBetween));
				continue;
			}
		}

		// Chance roll.
		const float Roll = FMath::FRand();
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::White,
			FString::Printf(TEXT("[WorldEventManager] '%s' rolled %.2f (need <= %.2f)."),
				*Event.EventID.ToString(), Roll, Event.SpawnChance));
		if (Roll > Event.SpawnChance) continue;

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

	const FVector SpawnLoc = GetSpawnLocation(Player->GetActorLocation());
	ActiveTrader = GetWorld()->SpawnActor<ANPCActor>(Event.TraderClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (ActiveTrader)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[WorldEventManager] TravellingTrader '%s' spawned at X=%.0f."),
			*Event.EventID.ToString(), SpawnLoc.X);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
			FString::Printf(TEXT("[WorldEventManager] Trader spawned at (%.0f, %.0f, %.0f)."),
				SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
			FString::Printf(TEXT("[WorldEventManager] Trader FAILED to spawn at (%.0f, %.0f, %.0f)."),
				SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z));
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
		const FVector SpawnLoc = GetSpawnLocation(Player->GetActorLocation());
		if (GetWorld()->SpawnActor<ACharacter>(Event.EnemyClass, SpawnLoc, FRotator::ZeroRotator, Params))
		{
			++Spawned;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[WorldEventManager] ScavengerRaid '%s' — %d/%d enemies spawned."),
		*Event.EventID.ToString(), Spawned, Count);
	GEngine->AddOnScreenDebugMessage(-1, 10.f, Spawned > 0 ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("[WorldEventManager] Raid: %d/%d enemies spawned."), Spawned, Count));
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
			const FVector SpawnLoc = GetSpawnLocation(Player->GetActorLocation());

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
	GEngine->AddOnScreenDebugMessage(-1, 10.f, Spawned > 0 ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("[WorldEventManager] Crate: %d items spawned (%d rolls)."), Spawned, Rolls));
}

// ── Utilities ────────────────────────────────────────────────────────────────

ABaseCharacter* AWorldEventManager::GetPlayerCharacter() const
{
	return Cast<ABaseCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

FVector AWorldEventManager::GetSpawnLocation(const FVector& Origin) const
{
	// Prefer a randomly chosen named spawn point placed in the level.
	if (SpawnPoints.Num() > 0)
	{
		const int32 Idx = FMath::RandRange(0, SpawnPoints.Num() - 1);
		if (SpawnPoints[Idx].Get())
		{
			const FVector Loc = SpawnPoints[Idx]->GetActorLocation();
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan,
				FString::Printf(TEXT("[WorldEventManager] Using spawn point '%s' at (%.0f, %.0f, %.0f)."),
					*SpawnPoints[Idx]->SpawnPointID.ToString(), Loc.X, Loc.Y, Loc.Z));
			return Loc;
		}
	}

	// Fallback: scatter on the X axis around Origin (same as before).
	const float OffsetX = FMath::FRandRange(-SpawnScatterRadius, SpawnScatterRadius);
	const FVector Fallback = FVector(Origin.X + OffsetX, Origin.Y, Origin.Z);
	GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
		FString::Printf(TEXT("[WorldEventManager] No spawn points — using fallback at (%.0f, %.0f, %.0f)."),
			Fallback.X, Fallback.Y, Fallback.Z));
	return Fallback;
}
