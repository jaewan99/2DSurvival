// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/BuildingEntrance.h"
#include "World/ExitSpawnPoint.h"

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset)
{
	if (!StartStreet) return;

	PendingStreet         = StartStreet;
	PendingOffset         = WorldOffset;
	PendingTransitionType = ETransitionType::Street;
	PendingIncomingExitID = FName("Start");  // place AExitSpawnPoint SpawnID="Start" in starting street
	StreamingToUnload     = nullptr;

	StartingStreetDef = StartStreet;
	VisitedStreetIDs.Add(StartStreet->StreetID);

	LoadStreet(StartStreet, WorldOffset);
}

void UStreetManager::OnPlayerCrossedExit(FName ExitID)
{
	if (bTransitionInProgress) return;
	if (!CurrentStreet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] OnPlayerCrossedExit: CurrentStreet not set yet."));
		return;
	}

	const FStreetExitLink* Link = CurrentStreet->GetExit(ExitID);
	if (!Link || !Link->Destination)
	{
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exit '%s' is blocked or not defined on '%s'."),
			*ExitID.ToString(), *CurrentStreet->StreetID.ToString());
		return;
	}

	bTransitionInProgress = true;

	if (Link->Layout == EExitLayout::Building)
	{
		// ── Enter building ───────────────────────────────────────────────────
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Entering building '%s' via exit '%s'."),
			*Link->Destination->StreetID.ToString(), *ExitID.ToString());

		ReturnStreet         = CurrentStreet;
		ReturnStreetOffset   = CurrentStreetWorldOffset;
		ReturnPlayerLocation = GetPlayerLocation();
		PendingIncomingExitID = ExitID;

		PendingTransitionType = ETransitionType::EnterBuilding;
		StreamingToUnload     = ActiveStreaming;
		PendingStreet         = Link->Destination;
		PendingOffset         = FVector(BuildingWorldX, 0.f, 0.f);

		LoadStreet(Link->Destination, FVector(BuildingWorldX, 0.f, 0.f));
	}
	else
	{
		// ── Adjacent street (walk-through) ───────────────────────────────────
		const FVector NextOffset = ComputeAdjacentOffset(Link->Layout, Link->Destination);

		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Crossing exit '%s' → '%s' at X=%.0f."),
			*ExitID.ToString(), *Link->Destination->StreetID.ToString(), NextOffset.X);

		PendingTransitionType = ETransitionType::Street;
		PendingIncomingExitID = ExitID;  // used to find AExitSpawnPoint in the new street
		StreamingToUnload     = ActiveStreaming;
		PendingStreet         = Link->Destination;
		PendingOffset         = NextOffset;

		LoadStreet(Link->Destination, NextOffset);
	}
}

void UStreetManager::OnPlayerExitBuilding()
{
	if (bTransitionInProgress) return;
	if (!bIsInsideBuilding)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] OnPlayerExitBuilding called but not inside a building."));
		return;
	}
	if (!ReturnStreet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] OnPlayerExitBuilding: no ReturnStreet saved."));
		return;
	}

	bTransitionInProgress = true;

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exiting building — returning to '%s'."),
		*ReturnStreet->StreetID.ToString());

	PendingTransitionType = ETransitionType::ExitBuilding;
	StreamingToUnload     = ActiveStreaming;
	PendingStreet         = ReturnStreet;
	PendingOffset         = ReturnStreetOffset;

	LoadStreet(ReturnStreet, ReturnStreetOffset);
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

	if (CurrentStreet && !CurrentStreet->StreetID.IsNone())
		VisitedStreetIDs.Add(CurrentStreet->StreetID);

	// Detect city change and broadcast if it changed.
	UCityDefinition* NewCity = CurrentStreet ? CurrentStreet->OwnerCity.Get() : nullptr;
	if (NewCity != CurrentCity.Get())
	{
		CurrentCity = NewCity;
		OnCityChanged.Broadcast();
	}

	PendingStreaming          = nullptr;
	PendingStreet             = nullptr;
	bTransitionInProgress     = false;

	ULevel* LoadedLevel = ActiveStreaming ? ActiveStreaming->GetLoadedLevel() : nullptr;

	// ── Post-load behaviour per transition type ───────────────────────────────
	switch (PendingTransitionType)
	{
	case ETransitionType::EnterBuilding:
		bIsInsideBuilding = true;
		TeleportPlayerToSpawnPoint(LoadedLevel, PendingIncomingExitID);
		PendingIncomingExitID = NAME_None;

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
		if (!PendingIncomingExitID.IsNone())
		{
			TeleportPlayerToSpawnPoint(LoadedLevel, PendingIncomingExitID);
			PendingIncomingExitID = NAME_None;
		}
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Now in '%s' at X=%.0f."),
			*CurrentStreet->StreetID.ToString(), CurrentStreetWorldOffset.X);
		break;
	}

	PendingTransitionType = ETransitionType::Street;
	OnStreetChanged.Broadcast();
}

FVector UStreetManager::ComputeAdjacentOffset(EExitLayout Layout, UStreetDefinition* AdjacentStreet) const
{
	FVector Offset = CurrentStreetWorldOffset;

	switch (Layout)
	{
	case EExitLayout::AdjacentRight:
		Offset.X += CurrentStreet->StreetWidth;
		break;

	case EExitLayout::AdjacentLeft:
		Offset.X -= AdjacentStreet->StreetWidth;
		break;

	default:
		break;
	}

	return Offset;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — teleport helpers
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::TeleportPlayerToSpawnPoint(ULevel* Level, FName IncomingExitID)
{
	if (!Level) return;

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return;

	// First: look for an AExitSpawnPoint with a matching SpawnID.
	for (AActor* Actor : Level->Actors)
	{
		if (AExitSpawnPoint* Spawn = Cast<AExitSpawnPoint>(Actor))
		{
			if (Spawn->SpawnID == IncomingExitID)
			{
				PC->GetPawn()->SetActorLocation(
					Spawn->GetActorLocation(),
					/*bSweep=*/false,
					/*OutSweepHitResult=*/nullptr,
					ETeleportType::TeleportPhysics);

				UE_LOG(LogTemp, Log, TEXT("[StreetManager] Teleported player to spawn point '%s' at %s."),
					*IncomingExitID.ToString(), *Spawn->GetActorLocation().ToString());
				return;
			}
		}
	}

	// Fallback: use the first ABuildingEntrance in the level (backward compatibility).
	for (AActor* Actor : Level->Actors)
	{
		if (ABuildingEntrance* Entrance = Cast<ABuildingEntrance>(Actor))
		{
			PC->GetPawn()->SetActorLocation(
				Entrance->GetActorLocation(),
				/*bSweep=*/false,
				/*OutSweepHitResult=*/nullptr,
				ETeleportType::TeleportPhysics);

			UE_LOG(LogTemp, Warning,
				TEXT("[StreetManager] No AExitSpawnPoint with SpawnID='%s' found — fell back to ABuildingEntrance."),
				*IncomingExitID.ToString());
			return;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[StreetManager] TeleportPlayerToSpawnPoint: no AExitSpawnPoint or ABuildingEntrance found in level."));
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
