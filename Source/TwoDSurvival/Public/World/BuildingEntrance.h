// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BuildingEntrance.generated.h"

class UBoxComponent;

/**
 * Placed at the entrance/exit of a building sublevel (and optionally on the street
 * in front of the building door).
 *
 * When the player presses E:
 *   - If on the street side → UStreetManager enters the building (loads building sublevel)
 *   - If inside the building → UStreetManager exits the building (restores the street)
 *   The manager distinguishes the two cases via its bIsInsideBuilding flag.
 *
 * Also serves as the player spawn point inside the building:
 *   UStreetManager scans for this actor after OnLevelShown and teleports the player here.
 *
 * Blueprint child (BP_BuildingEntrance):
 *   - Assign a door mesh
 *   - Resize InteractionBox to cover the doorway
 */
UCLASS()
class TWODSURVIVAL_API ABuildingEntrance : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABuildingEntrance();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override { return EInteractionType::Instant; }
	virtual float GetInteractionDuration_Implementation() override { return 0.f; }
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;
};
