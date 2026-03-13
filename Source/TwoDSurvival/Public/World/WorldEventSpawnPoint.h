// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldEventSpawnPoint.generated.h"

/**
 * Place in a street sublevel to define where actors spawn when that street loads.
 *
 * On BeginPlay (i.e. when the sublevel streams in), spawns ActorToSpawn at this location.
 * Use Left1, Right1, Top, Bottom etc. as SpawnPointID labels for your own reference.
 *
 * Blueprint child (BP_WorldEventSpawnPoint):
 *   - Set SpawnPointID (label only, for your reference).
 *   - Set ActorToSpawn to the enemy/NPC Blueprint you want at this location.
 *   - Leave ActorToSpawn empty for a point used only by WorldEventManager at night.
 */
UCLASS()
class TWODSURVIVAL_API AWorldEventSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AWorldEventSpawnPoint();

	/** Label for this point — e.g. "Left1", "Right1", "Top". For your reference only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	FName SpawnPointID = NAME_None;

	/**
	 * Pool of actor classes eligible to spawn here when the street loads.
	 * One is chosen at random. Leave empty to use this point only as a
	 * location marker for WorldEventManager night events.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TArray<TSubclassOf<AActor>> SpawnPool;

	/**
	 * Probability (0–1) that anything spawns here when the street loads.
	 * 1.0 = always, 0.5 = 50% chance, 0.0 = never (location marker only).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 1.0f;

protected:
	virtual void BeginPlay() override;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UArrowComponent> EditorArrow;
#endif
};
