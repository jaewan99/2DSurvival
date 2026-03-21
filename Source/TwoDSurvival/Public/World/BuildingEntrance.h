// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BuildingEntrance.generated.h"

class UBoxComponent;

/**
 * Press-E interactable placed at a building door.
 *
 * Teleports the player between the street Y layer (0) and the building interior Y layer (InteriorY).
 * Does NOT manage indoors state or facade visibility — ABuildingInteriorVolume handles both
 * automatically for any entry path (door, roof jump, underground, etc.).
 *
 * Setup:
 *   - Place BP_BuildingEntrance at each door opening.
 *   - Set InteriorY to match the Y position of the interior geometry.
 *   - The InteractionBox auto-sizes in BeginPlay to cover both the street and interior layers.
 *
 * Blueprint child (BP_BuildingEntrance):
 *   - Assign a door mesh
 *   - Set InteriorY in Defaults
 */
UCLASS()
class TWODSURVIVAL_API ABuildingEntrance : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABuildingEntrance();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	/**
	 * Y world position of the building interior layer.
	 * Must match the Y coordinate of the interior geometry in the sublevel.
	 * Adjust in BP_BuildingEntrance defaults.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building")
	float InteriorY = 200.f;

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
	bool bPendingEntering = false;
	float PendingTargetY = 0.f;

	FTimerHandle MidFadeTimer;
	FTimerHandle EndFadeTimer;

	void OnMidFade();
	void OnFadeComplete();
};
