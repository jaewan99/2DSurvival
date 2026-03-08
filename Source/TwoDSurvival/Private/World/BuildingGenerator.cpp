// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingGenerator.h"
#include "World/BuildingDefinition.h"
#include "World/RoomDefinition.h"
#include "World/RoomCell.h"
#include "World/VerticalTransport.h"
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

	const UBuildingDefinition* Def = BuildingDef;
	const FVector Origin = GetActorLocation();

	const int32 Seed = (Def->RandomSeed != 0) ? Def->RandomSeed : FMath::Rand();
	FRandomStream Stream(Seed);

	for (int32 Floor = 0; Floor < Def->FloorCount; Floor++)
	{
		const float LocalZ    = Floor * Def->FloorHeight;
		const bool  bTopFloor = (Floor == Def->FloorCount - 1);

		// Clamp to last defined layout if FloorLayouts has fewer entries than FloorCount
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
			const float   LocalX   = Room * Def->RoomWidth;
			const FVector WorldPos = Origin + FVector(LocalX, 0.f, LocalZ);
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

			// Determine which category this slot requires
			ERoomCategory Required = ERoomCategory::Any;
			if (Layout && Layout->SlotCategories.Num() > 0)
			{
				// Clamp to last defined slot if SlotCategories has fewer entries than RoomsPerFloor
				const int32 SlotIdx = FMath::Min(Room, Layout->SlotCategories.Num() - 1);
				Required = Layout->SlotCategories[SlotIdx];
			}

			// Filter RoomPool by category
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

			// Pick a random candidate and avoid repeating the same archetype twice in a row
			URoomDefinition* Chosen = Candidates[Stream.RandRange(0, Candidates.Num() - 1)];

			AActor* Spawned = SpawnRoomAt(Chosen->RoomActorClass, WorldPos);
			if (ARoomCell* RoomCell = Cast<ARoomCell>(Spawned))
			{
				RoomCell->SetRoomDimensions(Def->RoomWidth, Def->FloorHeight);
				RoomCell->ApplyRoomDefinition(Chosen);
			}
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

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(ActorClass, WorldPosition, FRotator::ZeroRotator, Params);
	if (Spawned)
	{
		if (!GetWorld()->IsGameWorld())
			Spawned->SetFlags(RF_Transient);
		SpawnedActors.Add(Spawned);
	}
	return Spawned;
}
