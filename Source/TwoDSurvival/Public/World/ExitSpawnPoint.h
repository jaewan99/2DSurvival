// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExitSpawnPoint.generated.h"

/**
 * Marks the spawn location for a player entering a sublevel via a specific exit.
 *
 * Place one of these inside any sublevel that is loaded via a Building-layout exit.
 * Set SpawnID to match the ExitID on the FStreetExitLink that points to this level.
 *
 * Example:
 *   DA_Street_Market has an exit { ExitID="FrontDoor", Layout=Building, Destination=DA_Building_Market }
 *   Inside DA_Building_Market's sublevel, place BP_ExitSpawnPoint with SpawnID="FrontDoor".
 *   When the player enters via "FrontDoor", UStreetManager teleports them here.
 *
 * Multiple spawn points can coexist in the same sublevel — one per entrance.
 *
 * Blueprint child (BP_ExitSpawnPoint):
 *   - Set SpawnID in Details.
 *   - The arrow indicator is editor-only and hidden at runtime.
 */
UCLASS()
class TWODSURVIVAL_API AExitSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AExitSpawnPoint();

	/**
	 * Matches the ExitID on the FStreetExitLink that leads to this level.
	 * UStreetManager scans all AExitSpawnPoints in the loaded level and teleports
	 * the player to the one whose SpawnID matches the incoming exit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	FName SpawnID = NAME_None;

#if WITH_EDITORONLY_DATA
	// Arrow shown in the editor viewport to make the spawn point easy to locate and orient.
	UPROPERTY()
	TObjectPtr<class UArrowComponent> EditorArrow;
#endif
};
