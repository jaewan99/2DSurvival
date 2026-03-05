// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingGenerator.h"
#include "World/BuildingDefinition.h"
#include "Engine/World.h"

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
		UE_LOG(LogTemp, Warning, TEXT("[BuildingGenerator] Generate called but BuildingDef is null on '%s'."), *GetName());
		return;
	}

	// Destroy any actors from a previous Generate() call
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	SpawnedActors.Empty();

	const FVector Origin = GetActorLocation();

	for (int32 Floor = 0; Floor < BuildingDef->FloorCount; Floor++)
	{
		for (int32 Room = 0; Room < BuildingDef->RoomsPerFloor; Room++)
		{
			const float LocalX = Room * BuildingDef->RoomWidth;
			const float LocalZ = Floor * BuildingDef->FloorHeight;
			const FVector WorldPos = Origin + FVector(LocalX, 0.f, LocalZ);

			// Pick which actor class to spawn at this cell
			TSubclassOf<AActor> ClassToSpawn = BuildingDef->RoomActorClass;

			if (FMath::IsNearlyEqual(LocalX, BuildingDef->StairX, 1.f))
			{
				ClassToSpawn = BuildingDef->StairsActorClass;
			}
			else if (BuildingDef->bHasElevator && FMath::IsNearlyEqual(LocalX, BuildingDef->ElevatorX, 1.f))
			{
				ClassToSpawn = BuildingDef->ElevatorRoomActorClass;
			}

			if (ClassToSpawn)
			{
				SpawnRoomAt(ClassToSpawn, WorldPos);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BuildingGenerator] Generated %d rooms for '%s' (%d floors × %d rooms)."),
		SpawnedActors.Num(), *GetName(), BuildingDef->FloorCount, BuildingDef->RoomsPerFloor);
}

void ABuildingGenerator::SpawnRoomAt(TSubclassOf<AActor> ActorClass, FVector WorldPosition)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(ActorClass, WorldPosition, FRotator::ZeroRotator, Params);
	if (Spawned)
	{
		// Transient in editor so preview actors don't get saved into the level file.
		// At runtime (IsGameWorld) they're normal persistent actors.
		if (!GetWorld()->IsGameWorld())
		{
			Spawned->SetFlags(RF_Transient);
		}
		SpawnedActors.Add(Spawned);
	}
}
