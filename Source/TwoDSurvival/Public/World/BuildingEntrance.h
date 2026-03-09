// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BuildingEntrance.generated.h"

class UBoxComponent;

/**
 * Press-E interactable placed at a building entrance, both on the street side and inside the building.
 *
 * Street side:
 *   - Player presses E → UStreetManager loads the building sublevel and teleports the player
 *     to the AExitSpawnPoint whose SpawnID matches BuildingExitID.
 *   - Set BuildingExitID to match a FStreetExitLink::ExitID (Layout=Building) on the current street.
 *
 * Building side (inside the building sublevel):
 *   - Player presses E → UStreetManager exits the building and restores the street.
 *   - BuildingExitID is ignored on this side (exit state is saved by the manager on entry).
 *
 * Blueprint child (BP_BuildingEntrance):
 *   - Assign a door mesh
 *   - Resize InteractionBox to cover the doorway
 *   - Set BuildingExitID in Details (street-side actor only; building-side actor can leave it blank)
 */
UCLASS()
class TWODSURVIVAL_API ABuildingEntrance : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABuildingEntrance();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	/**
	 * Must match a FStreetExitLink::ExitID (with Layout=Building) on this street's UStreetDefinition.
	 * Only used when entering a building from the street side.
	 * Example: if DA_Street_Market has exit { ExitID="FrontDoor", Layout=Building }, set this to "FrontDoor".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FName BuildingExitID = NAME_None;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override { return EInteractionType::Instant; }
	virtual float GetInteractionDuration_Implementation() override { return 0.f; }
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;
};
