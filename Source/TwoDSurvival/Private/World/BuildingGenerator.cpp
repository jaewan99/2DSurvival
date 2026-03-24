// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingGenerator.h"
#include "World/BuildingDefinition.h"
#include "World/RoomDefinition.h"
#include "World/RoomCell.h"
#include "World/VerticalTransport.h"
#include "World/BuildingInteriorVolume.h"
#include "World/BuildingFacadePanel.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

ABuildingGenerator::ABuildingGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABuildingGenerator::BeginPlay()
{
	Super::BeginPlay();
	Generate();
}

void ABuildingGenerator::Generate()
{
	if (!BuildingDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuildingGenerator] BuildingDef is null on '%s'."), *GetName());
		return;
	}

	for (AActor* Actor : SpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	SpawnedActors.Empty();
	InteriorVolume = nullptr;

	const UBuildingDefinition* Def = BuildingDef;

	const int32 Seed = (Def->RandomSeed != 0) ? Def->RandomSeed : FMath::Rand();
	FRandomStream Stream(Seed);

	for (int32 Floor = 0; Floor < Def->FloorCount; Floor++)
	{
		const float LocalZ = Floor * Def->FloorHeight;
		const bool  bTopFloor = (Floor == Def->FloorCount - 1);

		const int32 LayoutIdx = FMath::Min(Floor, Def->FloorLayouts.Num() - 1);
		const FFloorLayout* Layout = (LayoutIdx >= 0) ? &Def->FloorLayouts[LayoutIdx] : nullptr;

		// --- Stair placement ---
		TSet<int32> StairRooms;
		if (!bTopFloor)
		{
			if (Def->bRandomizeStairs)
			{
				const int32 NumStairs = FMath::Clamp(
					Stream.RandRange(Def->MinStairsPerFloor, Def->MaxStairsPerFloor),
					1, Def->RoomsPerFloor);

				TArray<int32> Indices;
				for (int32 i = 0; i < Def->RoomsPerFloor; i++) Indices.Add(i);
				for (int32 i = Indices.Num() - 1; i > 0; i--)
				{
					Indices.Swap(i, Stream.RandRange(0, i));
				}
				for (int32 i = 0; i < NumStairs; i++) StairRooms.Add(Indices[i]);
			}
			else
			{
				const int32 FixedRoom = FMath::RoundToInt(Def->StairX / Def->RoomWidth);
				if (FixedRoom >= 0 && FixedRoom < Def->RoomsPerFloor)
					StairRooms.Add(FixedRoom);
			}
		}

		// --- Spawn rooms ---
		for (int32 Room = 0; Room < Def->RoomsPerFloor; Room++)
		{
			const float   LocalX       = Room * Def->RoomWidth;
			const FVector RotatedHoriz = GetActorRotation().RotateVector(FVector(LocalX, 0.f, 0.f));
			const FVector WorldPos     = GetActorLocation() + RotatedHoriz + FVector(0.f, 0.f, LocalZ);
			const bool    bIsStair = StairRooms.Contains(Room);
			const bool    bIsElev  = Def->bHasElevator &&
			                         FMath::IsNearlyEqual(LocalX, Def->ElevatorX, 1.f);

			if (bIsStair && Def->StairsActorClass)
			{
				AActor* Spawned = SpawnRoomAt(Def->StairsActorClass, WorldPos);
				if (AVerticalTransport* VT = Cast<AVerticalTransport>(Spawned))
					VT->SetFloorHeight(Def->FloorHeight);
				continue;
			}

			if (bIsElev && Def->ElevatorRoomActorClass)
			{
				SpawnRoomAt(Def->ElevatorRoomActorClass, WorldPos);
				continue;
			}

			// Ground floor entrances — leftmost and rightmost slots are always doors.
			if (Floor == 0)
			{
				const bool bIsLeft  = (Room == 0);
				const bool bIsRight = (Room == Def->RoomsPerFloor - 1);

				if (bIsLeft && Def->LeftEntranceActorClass)
				{
					AActor* Spawned = SpawnRoomAt(Def->LeftEntranceActorClass, WorldPos);
					if (ARoomCell* Cell = Cast<ARoomCell>(Spawned))
						Cell->SetRoomDimensions(Def->RoomWidth);
					continue;
				}

				if (bIsRight && Def->RightEntranceActorClass)
				{
					AActor* Spawned = SpawnRoomAt(Def->RightEntranceActorClass, WorldPos);
					if (ARoomCell* Cell = Cast<ARoomCell>(Spawned))
						Cell->SetRoomDimensions(Def->RoomWidth);
					continue;
				}
			}

			ERoomCategory Required = ERoomCategory::Any;
			if (Layout && Layout->SlotCategories.Num() > 0)
			{
				const int32 SlotIdx = FMath::Min(Room, Layout->SlotCategories.Num() - 1);
				Required = Layout->SlotCategories[SlotIdx];
			}

			TArray<URoomDefinition*> Candidates;
			for (URoomDefinition* RoomDef : Def->RoomPool)
			{
				if (!RoomDef || !RoomDef->RoomActorClass) continue;
				if (Required == ERoomCategory::Any || RoomDef->Category == Required)
					Candidates.Add(RoomDef);
			}

			if (Candidates.Num() == 0)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[BuildingGenerator] No room in pool matches category %d for floor %d slot %d on '%s'."),
					(int32)Required, Floor, Room, *GetName());
				continue;
			}

			URoomDefinition* Chosen = Candidates[Stream.RandRange(0, Candidates.Num() - 1)];

			AActor* Spawned = SpawnRoomAt(Chosen->RoomActorClass, WorldPos);
			if (ARoomCell* RoomCell = Cast<ARoomCell>(Spawned))
			{
				RoomCell->SetRoomDimensions(Def->RoomWidth);
				RoomCell->ApplyRoomDefinition(Chosen);
			}

			// Spawn props from the room's PropSpawnTable.
			for (const FRoomPropEntry& Entry : Chosen->PropSpawnTable)
			{
				if (!Entry.ActorClass) continue;
				if (Entry.SpawnChance < 1.f && Stream.GetFraction() > Entry.SpawnChance) continue;

				const FVector PropWorld = WorldPos + GetActorRotation().RotateVector(Entry.RelativeOffset);
				SpawnRoomAt(Entry.ActorClass, PropWorld);
			}
		}
	}

	const float TotalWidth  = Def->RoomsPerFloor * Def->RoomWidth;
	const float TotalHeight = Def->FloorCount * Def->FloorHeight;

	// ── Spawn one facade panel per floor ──────────────────────────────────────
	TArray<ABuildingFacadePanel*> FacadePanels;
	if (FacadePanelClass)
	{
		for (int32 Floor = 0; Floor < Def->FloorCount; Floor++)
		{
			const float   LocalZ       = Floor * Def->FloorHeight;
			const FVector RotatedHoriz = GetActorRotation().RotateVector(FVector(TotalWidth * 0.5f, 0.f, 0.f));
			const FVector PanelPos     = GetActorLocation() + RotatedHoriz + FVector(0.f, 0.f, LocalZ);

			AActor* Spawned = SpawnRoomAt(FacadePanelClass, PanelPos);
			if (ABuildingFacadePanel* Panel = Cast<ABuildingFacadePanel>(Spawned))
			{
				Panel->InitPanel(Floor, TotalWidth, Def->FloorHeight);
				FacadePanels.Add(Panel);
			}
		}
	}

	// ── Spawn interior volume ─────────────────────────────────────────────────
	// Covers all floors but NOT the rooftop — player on rooftop won't trigger "inside".
	{
		const FVector RotatedHoriz = GetActorRotation().RotateVector(FVector(TotalWidth * 0.5f, 0.f, 0.f));
		const FVector VolumeCenter = GetActorLocation() + RotatedHoriz + FVector(0.f, 0.f, TotalHeight * 0.5f);

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABuildingInteriorVolume* Vol = GetWorld()->SpawnActor<ABuildingInteriorVolume>(
			ABuildingInteriorVolume::StaticClass(), VolumeCenter, GetActorRotation(), Params);

		if (Vol)
		{
			if (!GetWorld()->IsGameWorld())
				Vol->SetFlags(RF_Transient);

			Vol->GetTriggerBox()->SetBoxExtent(FVector(TotalWidth * 0.5f, 300.f, TotalHeight * 0.5f));
			Vol->SetFacadePanels(FacadePanels, GetActorLocation().Z, Def->FloorHeight);

			InteriorVolume = Vol;
			SpawnedActors.Add(Vol);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[BuildingGenerator] Generated %d actors for '%s' (%d floors x %d rooms, seed %d)."),
		SpawnedActors.Num(), *GetName(), Def->FloorCount, Def->RoomsPerFloor, Seed);
}

AActor* ABuildingGenerator::SpawnRoomAt(TSubclassOf<AActor> ActorClass, FVector WorldPosition)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(ActorClass, WorldPosition, GetActorRotation(), Params);
	if (Spawned)
	{
		if (!GetWorld()->IsGameWorld())
			Spawned->SetFlags(RF_Transient);
		SpawnedActors.Add(Spawned);
	}
	return Spawned;
}
