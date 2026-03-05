// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingGenerator.generated.h"

class UBuildingDefinition;

/**
 * Place one of these in every building sublevel.
 * Assign a UBuildingDefinition in the Details panel.
 *
 * UStreetManager calls Generate() after the sublevel finishes loading,
 * which spawns all room/stair/elevator actors in a grid based on the definition.
 *
 * Blueprint child (BP_BuildingGenerator):
 *   - No logic needed — just set BuildingDef in Details
 */
UCLASS()
class TWODSURVIVAL_API ABuildingGenerator : public AActor
{
	GENERATED_BODY()

public:
	ABuildingGenerator();

	// Assign this in the BP_BuildingGenerator Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UBuildingDefinition> BuildingDef;

	// Rebuilds the room grid. Callable from the Details panel button in-editor
	// (no Play required) and automatically from BeginPlay at runtime.
	UFUNCTION(CallInEditor, Category = "Building")
	void Generate();

protected:
	virtual void BeginPlay() override;

private:
	// Tracks every actor we spawned so Generate() can clean up on re-run.
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	void SpawnRoomAt(TSubclassOf<AActor> ActorClass, FVector WorldPosition);
};
