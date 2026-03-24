// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/BuildingDefinition.h"
#include "BuildingGenerator.generated.h"

class URoomDefinition;
class ARoomCell;
class ABuildingInteriorVolume;
class ABuildingFacadePanel;

/**
 * Place one of these in every building sublevel.
 * Assign a UBuildingDefinition and (optionally) a FacadePanelClass in the Details panel.
 *
 * UStreetManager calls Generate() after the sublevel finishes loading,
 * which spawns all room/stair/elevator actors in a grid based on the definition.
 * Generate() also spawns:
 *   - One ABuildingFacadePanel per floor (if FacadePanelClass is set), sized to the floor.
 *   - One ABuildingInteriorVolume covering all floors, which drives per-floor facade fading.
 *
 * Blueprint child (BP_BuildingGenerator):
 *   - Set BuildingDef in Details.
 *   - Optionally set FacadePanelClass to a BP_BuildingFacade child.
 */
UCLASS()
class TWODSURVIVAL_API ABuildingGenerator : public AActor
{
	GENERATED_BODY()

public:
	ABuildingGenerator();

	/** Assign in the BP_BuildingGenerator Details panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UBuildingDefinition> BuildingDef;

	/**
	 * Blueprint child of ABuildingFacadePanel — one instance is spawned per floor.
	 * Leave unset if this building has no facade (e.g. fully enclosed interiors).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TSubclassOf<ABuildingFacadePanel> FacadePanelClass;

	// Rebuilds the room grid. Callable from the Details panel button in-editor
	// (no Play required) and automatically from BeginPlay at runtime.
	UFUNCTION(CallInEditor, Category = "Building")
	void Generate();

protected:
	virtual void BeginPlay() override;

private:
	// All spawned actors — cleaned up on re-Generate.
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// The interior volume spawned by Generate() — sized to all floors (not rooftop).
	UPROPERTY()
	TObjectPtr<ABuildingInteriorVolume> InteriorVolume;

	AActor* SpawnRoomAt(TSubclassOf<AActor> ActorClass, FVector WorldPosition);
};
