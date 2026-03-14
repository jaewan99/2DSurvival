// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/BuildingDefinition.h"
#include "BuildingGenerator.generated.h"

class URoomDefinition;
class ARoomCell;
class ABuildingInteriorVolume;

/**
 * Place one of these in every building sublevel.
 * Assign a UBuildingDefinition in the Details panel.
 *
 * UStreetManager calls Generate() after the sublevel finishes loading,
 * which spawns all room/stair/elevator actors in a grid based on the definition.
 * Generate() also spawns an ABuildingInteriorVolume sized to the building interior
 * (all floors, NOT the rooftop) which drives facade fade-in/out automatically.
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

	/**
	 * Fades all room cell facades in or out over Duration seconds.
	 * Called by ABuildingInteriorVolume when the player enters or exits.
	 */
	void SetFacadesVisible(bool bVisible, float Duration = 0.35f);

protected:
	virtual void BeginPlay() override;

private:
	// All spawned actors — cleaned up on re-Generate.
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// Subset of SpawnedActors that are ARoomCell — used for facade fading.
	UPROPERTY()
	TArray<TObjectPtr<ARoomCell>> SpawnedRoomCells;

	// The interior volume spawned by Generate() — sized to all floors (not rooftop).
	UPROPERTY()
	TObjectPtr<ABuildingInteriorVolume> InteriorVolume;

	AActor* SpawnRoomAt(TSubclassOf<AActor> ActorClass, FVector WorldPosition);
};
