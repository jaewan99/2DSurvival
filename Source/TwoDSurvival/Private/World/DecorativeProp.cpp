// Fill out your copyright notice in the Description page of Project Settings.

#include "World/DecorativeProp.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ADecorativeProp::ADecorativeProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ADecorativeProp::BeginPlay()
{
	Super::BeginPlay();

	// ── Probability roll ──────────────────────────────────────────────────────
	if (FMath::FRand() > SpawnProbability)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		return;
	}

	// ── Variant selection ─────────────────────────────────────────────────────
	// If VariantMeshes is populated, pick one at random and apply it.
	if (VariantMeshes.Num() > 0)
	{
		const int32 Index = FMath::RandRange(0, VariantMeshes.Num() - 1);
		const TSoftObjectPtr<UStaticMesh>& Chosen = VariantMeshes[Index];
		if (!Chosen.IsNull())
		{
			UStaticMesh* Loaded = Chosen.LoadSynchronous();
			if (Loaded)
			{
				Mesh->SetStaticMesh(Loaded);
			}
		}
		// If the chosen entry is null, the default Blueprint mesh stays in place.
	}
}
