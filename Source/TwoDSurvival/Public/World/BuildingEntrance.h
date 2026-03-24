// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BuildingEntrance.generated.h"

class UBoxComponent;
class UArrowComponent;

/**
 * Press-E interactable placed at a building door — both outside and inside.
 *
 * Place two of these per doorway:
 *   - One on the STREET side  — move its Destination component to just inside the door
 *   - One on the INTERIOR side — move its Destination component to just outside the door
 *
 * No separate spawn point actors needed. The Destination scene component IS the
 * teleport target — drag it in the level editor to position it.
 *
 * ABuildingInteriorVolume handles bIsIndoors and facade fade automatically on overlap.
 *
 * Blueprint child (BP_BuildingEntrance):
 *   - Assign a door mesh
 *   - In the level, move the Destination component to the arrival position
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
	 * Teleport destination — move this arrow in the level editor to position
	 * where the player will appear after pressing E on this entrance.
	 * The arrow direction shows which way the player will face on arrival.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> Destination;

	/** Total duration of the fade-to-black transition (enter + exit combined). */
	UPROPERTY(EditDefaultsOnly, Category = "Building")
	float FadeTransitionDuration = 0.5f;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override { return EInteractionType::Instant; }
	virtual float GetInteractionDuration_Implementation() override { return 0.f; }
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

private:
	TWeakObjectPtr<ABaseCharacter> PendingInteractor;

	FTimerHandle MidFadeTimer;
	FTimerHandle EndFadeTimer;

	void OnMidFade();
	void OnFadeComplete();
};
