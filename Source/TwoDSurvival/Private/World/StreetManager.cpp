// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/BuildingEntrance.h"
#include "World/ExitSpawnPoint.h"
#include "World/CityDefinition.h"
#include "Character/BaseCharacter.h"
#include "Save/TwoDSurvivalSaveGame.h"
#include "AssetRegistry/AssetRegistryModule.h"

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

	// Generate city graph on fresh start. If LoadGame runs later it will overwrite this.
	if (!bCityGenerated)
		GenerateCityGraph();

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

	UStreetDefinition* Destination = ResolveExitDestination(CurrentStreet, ExitID);
	if (!Destination)
	{
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exit '%s' is blocked or not defined on '%s'."),
			*ExitID.ToString(), *CurrentStreet->StreetID.ToString());
		return;
	}

	// Determine layout: use declared exit if available, otherwise infer from convention.
	const FStreetExitLink* Link = CurrentStreet->GetExit(ExitID);
	EExitLayout Layout = EExitLayout::AdjacentRight;
	if (Link)
		Layout = Link->Layout;
	else if (ExitID == FName("Left"))
		Layout = EExitLayout::AdjacentLeft;

	bTransitionInProgress = true;

	if (Layout == EExitLayout::Building)
	{
		// ── Enter building ───────────────────────────────────────────────────
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Entering building '%s' via exit '%s'."),
			*Destination->StreetID.ToString(), *ExitID.ToString());

		ReturnStreet          = CurrentStreet;
		ReturnStreetOffset    = CurrentStreetWorldOffset;
		ReturnPlayerLocation  = GetPlayerLocation();
		PendingIncomingExitID = ExitID;

		PendingTransitionType = ETransitionType::EnterBuilding;
		StreamingToUnload     = ActiveStreaming;
		PendingStreet         = Destination;
		PendingOffset         = FVector(BuildingWorldX, 0.f, 0.f);

		LoadStreet(Destination, FVector(BuildingWorldX, 0.f, 0.f));
	}
	else
	{
		// ── Adjacent street (walk-through) ───────────────────────────────────
		const FVector NextOffset = ComputeAdjacentOffset(Layout, Destination);

		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Crossing exit '%s' → '%s' at X=%.0f."),
			*ExitID.ToString(), *Destination->StreetID.ToString(), NextOffset.X);

		PendingTransitionType = ETransitionType::Street;
		PendingIncomingExitID = ExitID;
		StreamingToUnload     = ActiveStreaming;
		PendingStreet         = Destination;
		PendingOffset         = NextOffset;

		LoadStreet(Destination, NextOffset);
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

				// Align movement axis and camera to the spawn point's orientation.
				// bFlipCamera toggles which side of the street the camera sits on.
				if (ABaseCharacter* Char = Cast<ABaseCharacter>(PC->GetPawn()))
				{
					const float SpawnYaw = Spawn->GetActorRotation().Yaw;
					const float CameraYaw = Spawn->bFlipCamera ? SpawnYaw + 90.f : SpawnYaw - 90.f;
					Char->SetMovementAxis(Spawn->GetActorForwardVector(), CameraYaw);
				}

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

// ─────────────────────────────────────────────────────────────────────────────
// Runtime city graph
// ─────────────────────────────────────────────────────────────────────────────

void UStreetManager::ScanAllStreetDefs()
{
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().WaitForCompletion();

	TArray<FAssetData> Assets;
	ARM.Get().GetAssetsByClass(UStreetDefinition::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& AD : Assets)
	{
		UStreetDefinition* Def = Cast<UStreetDefinition>(AD.GetAsset());
		if (Def && !Def->StreetID.IsNone())
			AllStreetDefsMap.Add(Def->StreetID, Def);
	}

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Scanned %d street definitions."), AllStreetDefsMap.Num());
}

void UStreetManager::GenerateCityGraph()
{
	ScanAllStreetDefs();

	// Group city streets (non-highway, non-building) by their OwnerCity.
	TMap<UCityDefinition*, TArray<UStreetDefinition*>> StreetsByCity;
	for (auto& KV : AllStreetDefsMap)
	{
		UStreetDefinition* Def = KV.Value.Get();
		if (!Def || !Def->OwnerCity || Def->bIsHighway || Def->bIsPCGBuilding) continue;
		StreetsByCity.FindOrAdd(Def->OwnerCity).Add(Def);
	}

	GeneratedGraph.Empty();

	for (auto& CityKV : StreetsByCity)
	{
		UCityDefinition* City = CityKV.Key;
		TArray<UStreetDefinition*>& Pool = CityKV.Value;
		if (Pool.IsEmpty()) continue;

		// Fisher-Yates shuffle
		for (int32 i = Pool.Num() - 1; i > 0; --i)
		{
			const int32 j = FMath::RandRange(0, i);
			Pool.Swap(i, j);
		}

		// Pick N streets clamped to pool size
		const int32 Min = FMath::Min(City->MinStreets, Pool.Num());
		const int32 Max = FMath::Min(FMath::Max(City->MaxStreets, Min), Pool.Num());
		const int32 N   = FMath::RandRange(Min, Max);
		Pool.SetNum(N);

		// Collect all unassigned, non-building exits per layout direction.
		// An exit is "unassigned" when its Destination is null — it wants a runtime neighbor.
		//
		// We use structs instead of raw pointers so exits from Left1/Left2/Right1/Right2/...
		// are all handled uniformly, regardless of name.
		struct FExitSlot { UStreetDefinition* Street; FName ExitID; };

		TArray<FExitSlot> RightSlots; // AdjacentRight exits wanting a neighbor
		TArray<FExitSlot> LeftSlots;  // AdjacentLeft  exits wanting a neighbor

		for (UStreetDefinition* S : Pool)
		{
			for (const FStreetExitLink& Exit : S->Exits)
			{
				if (Exit.Destination || Exit.Layout == EExitLayout::Building) continue;
				if (Exit.Layout == EExitLayout::AdjacentRight)
					RightSlots.Add({ S, Exit.ExitID });
				else
					LeftSlots.Add({ S, Exit.ExitID });
			}
		}

		// Shuffle both slot lists independently so pairings are random.
		auto Shuffle = [](TArray<FExitSlot>& Arr)
		{
			for (int32 i = Arr.Num() - 1; i > 0; --i)
				Arr.Swap(i, FMath::RandRange(0, i));
		};
		Shuffle(RightSlots);
		Shuffle(LeftSlots);

		// Pair each Right slot with a Left slot. Skip self-loops.
		// Any unpaired slots (city boundary streets) remain unassigned → blocked.
		int32 Li = 0;
		for (const FExitSlot& R : RightSlots)
		{
			// Advance past any left slot on the same street to avoid a self-loop.
			while (Li < LeftSlots.Num() && LeftSlots[Li].Street == R.Street)
				++Li;

			if (Li >= LeftSlots.Num()) break;

			const FExitSlot& L = LeftSlots[Li++];

			GeneratedGraph.FindOrAdd(R.Street->StreetID).Add(R.ExitID, L.Street->StreetID);
			GeneratedGraph.FindOrAdd(L.Street->StreetID).Add(L.ExitID, R.Street->StreetID);
		}

		UE_LOG(LogTemp, Log, TEXT("[StreetManager] City '%s': %d streets, %d right-slots, %d left-slots paired."),
			*City->CityName.ToString(), N, RightSlots.Num(), LeftSlots.Num());
	}

	bCityGenerated = true;
	UE_LOG(LogTemp, Log, TEXT("[StreetManager] City graph generated (%d street entries)."), GeneratedGraph.Num());
}

UStreetDefinition* UStreetManager::ResolveExitDestination(UStreetDefinition* Street, FName ExitID) const
{
	if (!Street) return nullptr;

	// 1. Hard-wired destination takes priority (building exits, highway exits, etc.)
	const FStreetExitLink* Link = Street->GetExit(ExitID);
	if (Link && Link->Destination)
		return Link->Destination.Get();

	// 2. Fall back to runtime graph
	const TMap<FName, FName>* ExitMap = GeneratedGraph.Find(Street->StreetID);
	if (ExitMap)
	{
		const FName* ToID = ExitMap->Find(ExitID);
		if (ToID && !ToID->IsNone())
		{
			const TObjectPtr<UStreetDefinition>* Found = AllStreetDefsMap.Find(*ToID);
			return Found ? Found->Get() : nullptr;
		}
	}

	return nullptr;
}

bool UStreetManager::HasResolvableExit(FName ExitID) const
{
	return ResolveExitDestination(CurrentStreet, ExitID) != nullptr;
}

UStreetDefinition* UStreetManager::FindStreetByID(FName StreetID) const
{
	const TObjectPtr<UStreetDefinition>* Found = AllStreetDefsMap.Find(StreetID);
	return Found ? Found->Get() : nullptr;
}

void UStreetManager::SaveGraphToSaveGame(UTwoDSurvivalSaveGame* Save) const
{
	if (!Save) return;

	Save->bStreetGraphGenerated = bCityGenerated;
	Save->SavedStreetGraph.Empty();

	for (const auto& StreetKV : GeneratedGraph)
	{
		for (const auto& ExitKV : StreetKV.Value)
		{
			FSavedStreetConnection Conn;
			Conn.FromStreetID = StreetKV.Key;
			Conn.ExitID       = ExitKV.Key;
			Conn.ToStreetID   = ExitKV.Value;
			Save->SavedStreetGraph.Add(Conn);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Saved %d street graph connections."), Save->SavedStreetGraph.Num());
}

void UStreetManager::RestoreGraphFromSaveGame(const UTwoDSurvivalSaveGame* Save)
{
	if (!Save || !Save->bStreetGraphGenerated) return;

	// Ensure the asset map is populated so ResolveExitDestination can look up defs by ID.
	if (AllStreetDefsMap.IsEmpty())
		ScanAllStreetDefs();

	GeneratedGraph.Empty();
	for (const FSavedStreetConnection& Conn : Save->SavedStreetGraph)
	{
		GeneratedGraph.FindOrAdd(Conn.FromStreetID).Add(Conn.ExitID, Conn.ToStreetID);
	}

	bCityGenerated = true;
	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Restored %d street graph connections from save."), Save->SavedStreetGraph.Num());
}
