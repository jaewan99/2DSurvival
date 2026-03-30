// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetPropScatter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AStreetPropScatter::AStreetPropScatter()
{
	PrimaryActorTick.bCanEverTick = false;

	ScatterZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ScatterZone"));
	ScatterZone->SetBoxExtent(FVector(2000.f, 50.f, 200.f));
	ScatterZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScatterZone->SetHiddenInGame(true);
	RootComponent = ScatterZone;
}

void AStreetPropScatter::BeginPlay()
{
	Super::BeginPlay();

	if (PropPool.IsEmpty()) return;

	const FVector Origin  = ScatterZone->GetComponentLocation();
	const FVector Extent  = ScatterZone->GetScaledBoxExtent();
	const float   FixedY  = Origin.Y; // keep all props on the same depth plane

	const int32 Count = FMath::RandRange(MinCount, FMath::Max(MinCount, MaxCount));

	for (int32 i = 0; i < Count; ++i)
	{
		const FPropScatterEntry* Entry = PickWeightedEntry();
		if (!Entry) continue;

		UStaticMesh* LoadedMesh = Entry->Mesh.LoadSynchronous();
		if (!LoadedMesh) continue;

		// Random XZ position within the zone; Y is fixed to the actor's depth.
		const float RandX = FMath::FRandRange(Origin.X - Extent.X, Origin.X + Extent.X);
		const float RandZ = FMath::FRandRange(Origin.Z - Extent.Z, Origin.Z + Extent.Z);
		const FVector SpawnPos(RandX, FixedY, RandZ);

		// Random uniform scale
		const float Scale = FMath::FRandRange(Entry->ScaleMin, Entry->ScaleMax);

		UStaticMeshComponent* SMC = NewObject<UStaticMeshComponent>(this);
		SMC->SetStaticMesh(LoadedMesh);
		SMC->SetWorldLocation(SpawnPos);
		SMC->SetWorldScale3D(FVector(Scale));
		SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SMC->SetMobility(EComponentMobility::Static);
		SMC->RegisterComponent();
		AddInstanceComponent(SMC);
	}
}

const FPropScatterEntry* AStreetPropScatter::PickWeightedEntry() const
{
	// Sum all weights
	float Total = 0.f;
	for (const FPropScatterEntry& E : PropPool)
		Total += FMath::Max(E.Weight, 0.01f);

	// Roll and walk down the list
	float Roll = FMath::FRandRange(0.f, Total);
	for (const FPropScatterEntry& E : PropPool)
	{
		Roll -= FMath::Max(E.Weight, 0.01f);
		if (Roll <= 0.f)
			return &E;
	}

	return &PropPool.Last();
}
