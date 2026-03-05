// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "DraggableProp.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * A physics-free slideable prop (chair, wheelie bin, crate, etc.) the player can grab and push.
 *
 * - Press E near the prop to grab it. Press E again to release.
 * - While grabbed, the prop is translated each tick to stay glued to the player's side.
 * - DragSpeedMultiplier reduces the player's MaxWalkSpeed while pushing.
 * - PropHalfExtentX sets the player→prop gap; match this to the mesh's actual X half-width.
 *
 * Blueprint child (BP_DraggableProp):
 *   - Assign a Static Mesh in the Mesh component.
 *   - Adjust CollisionBox / InteractionBox extents to match the mesh.
 *   - Set DragSpeedMultiplier and PropHalfExtentX in Class Defaults.
 */
UCLASS()
class TWODSURVIVAL_API ADraggableProp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ADraggableProp();

	// Visual mesh — assign in the Blueprint child Details panel.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	// World-blocking collision volume — keeps the prop solid against walls and other actors.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	// Overlap trigger used by UInteractionComponent's detection sphere to find this actor.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBox;

	// Fraction of MaxWalkSpeed applied to the player while this prop is being dragged.
	// 0.7 = 70% speed (player moves 30% slower while pushing).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drag")
	float DragSpeedMultiplier = 0.7f;

	// Half-width of the prop on the X axis (cm). Used to compute the side offset so the
	// player capsule and prop mesh sit edge-to-edge without overlapping.
	// Set this to match the mesh's actual X half-extent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drag")
	float PropHalfExtentX = 50.f;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;
};
