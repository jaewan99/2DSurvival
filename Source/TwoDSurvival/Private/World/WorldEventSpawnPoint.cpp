// Fill out your copyright notice in the Description page of Project Settings.

#include "World/WorldEventSpawnPoint.h"

#if WITH_EDITOR
#include "Components/ArrowComponent.h"
#endif

AWorldEventSpawnPoint::AWorldEventSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	// Orange arrow — distinct from the blue AExitSpawnPoint arrows.
	EditorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(RootComponent);
		EditorArrow->ArrowColor          = FColor(255, 140, 0);
		EditorArrow->ArrowSize           = 2.f;
		EditorArrow->bIsScreenSizeScaled = true;
		EditorArrow->SetHiddenInGame(true);
	}
#endif
}

void AWorldEventSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnPool.IsEmpty()) return;
	if (FMath::FRand() > SpawnChance) return;

	// Pick a random class from the pool.
	const int32 Idx = FMath::RandRange(0, SpawnPool.Num() - 1);
	TSubclassOf<AActor> ChosenClass = SpawnPool[Idx];
	if (!ChosenClass) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(ChosenClass, GetActorLocation(), GetActorRotation(), Params);
	if (Spawned)
	{
		UE_LOG(LogTemp, Log, TEXT("[SpawnPoint] '%s' spawned %s."), *SpawnPointID.ToString(), *ChosenClass->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("[SpawnPoint] '%s' spawned %s."), *SpawnPointID.ToString(), *ChosenClass->GetName()));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnPoint] '%s' failed to spawn %s."), *SpawnPointID.ToString(), *ChosenClass->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			FString::Printf(TEXT("[SpawnPoint] '%s' FAILED to spawn %s."), *SpawnPointID.ToString(), *ChosenClass->GetName()));
	}
}
