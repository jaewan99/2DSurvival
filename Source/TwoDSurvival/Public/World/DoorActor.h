// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "DoorActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

/**
 * A door placed between two rooms by ABuildingGenerator.
 *
 * Closed → mesh visible + collision blocking.
 * Open   → mesh hidden + collision off (player walks through freely).
 *
 * Blueprint child (BP_DoorActor):
 *   - Assign a door mesh in the Details panel (DoorMesh).
 *   - Optionally tweak InteractionBox extents.
 */
UCLASS()
class TWODSURVIVAL_API ADoorActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ADoorActor();

	// Visual door panel. Assign mesh in BP_DoorActor Details.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// Proximity trigger for the interaction system.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UBoxComponent> InteractionBox;

	// If true, the door cannot be opened by the player.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bIsLocked = false;

	// When true the door spawns already open.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bStartOpen = false;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsOpen = false;

	void ApplyOpenState();
};
