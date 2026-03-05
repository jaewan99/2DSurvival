// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/BuildingDefinition.h"
#include "World/BuildingEntrance.h"
#include "World/BuildingGenerator.h"

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset)
{
	if (!StartStreet) return;

	PendingStreet         = StartStreet;
	PendingOffset         = WorldOffset;
	PendingTransitionType = ETransitionType::Street;
	StreamingToUnload     = nullptr;

	LoadStreet(StartStreet, WorldOffset);
}

void UStreetManager::OnPlayerCrossedExit(EExitDirection Direction)
{
	if (bTransitionInProgress) return;
	if (!CurrentStreet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] OnPlayerCrossedExit: CurrentStreet not set yet."));
		return;
	}

	bTransitionInProgress = true;

	if (Direction == EExitDirection::Up)
	{
		if (bIsInsideBuilding)
		{
			// ── Exit building → return to street ────────────────────────────
			if (!ReturnStreet)
			{
				UE_LOG(LogTemp, Warning, TEXT("[StreetManager] ExitBuilding: no ReturnStreet saved."));
				bTransitionInProgress = false;
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exiting building — returning to '%s'."),
				*ReturnStreet->StreetID.ToString());

			PendingTransitionType = ETransitionType::ExitBuilding;
			StreamingToUnload     = ActiveStreaming;
			PendingStreet         = ReturnStreet;
			PendingOffset         = ReturnStreetOffset;

			LoadStreet(ReturnStreet, ReturnStreetOffset);
		}
		else
		{
			// ── Enter building ───────────────────────────────────────────────
			UStreetDefinition* Building = CurrentStreet->GetExit(EExitDirection::Up);
			if (!Building)
			{
				UE_LOG(LogTemp, Log, TEXT("[StreetManager] No building exit on '%s'."),
					*CurrentStreet->StreetID.ToString());
				bTransitionInProgress = false;
				return;
			}

			if (!Building->bIsPCGBuilding)
			{
				UE_LOG(LogTemp, Warning, TEXT("[StreetManager] ExitUp '%s' is not flagged as a PCG building."),
					*Building->StreetID.ToString());
				bTransitionInProgress = false;
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[StreetManager] Entering building '%s'."),
				*Building->StreetID.ToString());

			// Save state needed to return
			ReturnStreet         = CurrentStreet;
			ReturnStreetOffset   = CurrentStreetWorldOffset;
			ReturnPlayerLocation = GetPlayerLocation();

			PendingTransitionType = ETransitionType::EnterBuilding;
			StreamingToUnload     = ActiveStreaming;
			PendingStreet         = Building;
			PendingOffset         = FVector(BuildingWorldX, 0.f, 0.f);

			LoadStreet(Building, FVector(BuildingWorldX, 0.f, 0.f));
		}
		return;
	}

	// ── Left / Right ─────────────────────────────────────────────────────────
	UStreetDefinition* NextStreet = CurrentStreet->GetExit(Direction);
	if (!NextStreet)
	{
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exit %d is blocked — no adjacent street."), (int32)Direction);
		bTransitionInProgress = false;
		return;
	}

	const FVector NextOffset = ComputeAdjacentOffset(Direction, NextStreet);

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Player crossed %d — streaming '%s' at X=%.0f."),
		(int32)Direction, *NextStreet->StreetID.ToString(), NextOffset.X);

	PendingTransitionType = ETransitionType::Street;
	StreamingToUnload     = ActiveStreaming;
	PendingStreet         = NextStreet;
	PendingOffset         = NextOffset;

	LoadStreet(NextStreet, NextOffset);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — streaming
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::LoadStreet(UStreetDefinition* Street, FVector WorldOffset)
{
	if (!Street || Street->Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] LoadStreet: '%s' has no Level asset assigned."),
			Street ? *Street->StreetID.ToString() : TEXT("null"));
		bTransitionInProgress = false;
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	bool bSuccess = false;
	const FTransform LevelTransform(FQuat::Identity, WorldOffset, FVector::OneVector);

	ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		World, Street->Level, LevelTransform, bSuccess);

	if (!Streaming || !bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[StreetManager] LoadLevelInstanceBySoftObjectPtr failed for '%s'."),
			*Street->StreetID.ToString());
		bTransitionInProgress = false;
		return;
	}

	PendingStreaming = Streaming;
	Streaming->OnLevelShown.AddDynamic(this, &UStreetManager::OnNewStreetShown);
}

void UStreetManager::OnNewStreetShown()
{
	// Unbind immediately — this delegate fires once per load
	if (PendingStreaming)
	{
		PendingStreaming->OnLevelShown.RemoveDynamic(this, &UStreetManager::OnNewStreetShown);
	}

	// Unload the previous sublevel
	if (StreamingToUnload)
	{
		StreamingToUnload->SetShouldBeLoaded(false);
		StreamingToUnload->SetShouldBeVisible(false);
		StreamingToUnload = nullptr;
	}

	// Commit new state
	ActiveStreaming          = PendingStreaming;
	CurrentStreet            = PendingStreet;
	CurrentStreetWorldOffset = PendingOffset;
	PendingStreaming          = nullptr;
	PendingStreet             = nullptr;
	bTransitionInProgress     = false;

	ULevel* LoadedLevel = ActiveStreaming ? ActiveStreaming->GetLoadedLevel() : nullptr;

	// ── Post-load behaviour per transition type ───────────────────────────────
	switch (PendingTransitionType)
	{
	case ETransitionType::EnterBuilding:
		bIsInsideBuilding = true;
		TeleportPlayerToBuildingEntrance(LoadedLevel);

		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Entered building '%s'."),
			*CurrentStreet->StreetID.ToString());
		break;

	case ETransitionType::ExitBuilding:
		bIsInsideBuilding = false;
		TeleportPlayerToLocation(ReturnPlayerLocation);
		ReturnStreet = nullptr;

		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exited building — back on '%s' at X=%.0f."),
			*CurrentStreet->StreetID.ToString(), CurrentStreetWorldOffset.X);
		break;

	default:
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Now in '%s' at X=%.0f."),
			*CurrentStreet->StreetID.ToString(), CurrentStreetWorldOffset.X);
		break;
	}

	PendingTransitionType = ETransitionType::Street;
	OnStreetChanged.Broadcast();
}

FVector UStreetManager::ComputeAdjacentOffset(EExitDirection Direction, UStreetDefinition* AdjacentStreet) const
{
	FVector Offset = CurrentStreetWorldOffset;

	switch (Direction)
	{
	case EExitDirection::Right:
		Offset.X += CurrentStreet->StreetWidth;
		break;

	case EExitDirection::Left:
		Offset.X -= AdjacentStreet->StreetWidth;
		break;

	default:
		break;
	}

	return Offset;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — building helpers
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::TriggerBuildingGeneration(ULevel* Level)
{
	if (!Level) return;

	for (AActor* Actor : Level->Actors)
	{
		if (ABuildingGenerator* Generator = Cast<ABuildingGenerator>(Actor))
		{
			Generator->Generate();
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[StreetManager] TriggerBuildingGeneration: no ABuildingGenerator found in building level."));
}

void UStreetManager::TeleportPlayerToBuildingEntrance(ULevel* Level)
{
	if (!Level) return;

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	// Find the ABuildingEntrance actor placed in the building sublevel — it acts as the spawn point.
	for (AActor* Actor : Level->Actors)
	{
		if (ABuildingEntrance* Entrance = Cast<ABuildingEntrance>(Actor))
		{
			PC->GetPawn()->SetActorLocation(
				Entrance->GetActorLocation(),
				/*bSweep=*/false,
				/*OutSweepHitResult=*/nullptr,
				ETeleportType::TeleportPhysics);

			UE_LOG(LogTemp, Log, TEXT("[StreetManager] Teleported player to building entrance at %s."),
				*Entrance->GetActorLocation().ToString());
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[StreetManager] TeleportPlayerToBuildingEntrance: no ABuildingEntrance found in level."));
}

void UStreetManager::TeleportPlayerToLocation(FVector Location)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	PC->GetPawn()->SetActorLocation(
		Location,
		/*bSweep=*/false,
		/*OutSweepHitResult=*/nullptr,
		ETeleportType::TeleportPhysics);

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Teleported player to %s."), *Location.ToString());
}

FVector UStreetManager::GetPlayerLocation() const
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return FVector::ZeroVector;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return FVector::ZeroVector;

	return PC->GetPawn()->GetActorLocation();
}
