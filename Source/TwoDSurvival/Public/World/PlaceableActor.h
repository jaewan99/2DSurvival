// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "PlaceableActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UItemDefinition;
class ABaseCharacter;
class UMaterialInterface;

/**
 * Base class for all player-placeable furniture and decorations.
 * Subclass in Blueprint (e.g. BP_Chair, BP_Table) — assign a mesh in the viewport.
 *
 * Lifecycle:
 *   1. ABaseCharacter::PlaceItem() spawns this actor in ghost mode via SpawnActor.
 *   2. Each tick BaseCharacter moves it to the mouse-projected XZ position and calls
 *      SetGhostValid() to tint green (clear) or red (blocked).
 *   3. LMB confirm → FinalizePlace(): enables collision, records PlacementID.
 *   4. Press E on a placed actor → OnInteract: returns item to inventory, destroys self.
 *
 * Blueprint child needs:
 *   - Assign a Static Mesh to the Mesh component in the viewport.
 *   - Optionally resize the InteractionBox to match the mesh footprint.
 */
UCLASS()
class TWODSURVIVAL_API APlaceableActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	APlaceableActor();

	// Mesh displayed in world and during ghost preview.
	// Assign in the Blueprint child's viewport.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Proximity box used by UInteractionComponent so the player can press E to pick up.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	/**
	 * Unique ID generated the first time this actor is placed.
	 * Preserved through save/load so actors can be identified across sessions.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	FGuid PlacementID;

	/** Item definition this actor was created from — used to return the item on pickup. */
	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UItemDefinition> SourceItemDef;

	/**
	 * True while the actor is in ghost/preview mode (before LMB confirmation).
	 * Ghost actors have collision disabled and are excluded from saves.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	bool bIsGhost = false;

	/**
	 * Switches between ghost preview and a fully placed actor.
	 * Call with bGhost=true immediately after spawning.
	 * Call with bGhost=false (via FinalizePlace) on LMB confirm.
	 *
	 * @param bGhost         If true: disables collision and applies GhostMaterial to all slots.
	 *                       If false: re-enables collision and clears material overrides.
	 * @param GhostMaterial  Material applied to all mesh slots when entering ghost mode.
	 *                       Pass nullptr to skip material override (e.g. when restoring).
	 */
	void SetGhostMode(bool bGhost, UMaterialInterface* GhostMaterial = nullptr);

	/**
	 * Switches the ghost material between the valid (green) and invalid (red) variants.
	 * No-op when bIsGhost is false.
	 *
	 * @param bValid             True = placement is clear; apply ValidMaterial.
	 * @param ValidMaterial      Material to apply when placement is clear.
	 * @param InvalidMaterial    Material to apply when placement is blocked.
	 */
	void SetGhostValid(bool bValid,
		UMaterialInterface* ValidMaterial,
		UMaterialInterface* InvalidMaterial);

	/**
	 * Converts the ghost into a fully placed actor:
	 *   - Assigns SourceItemDef so the actor knows what item to return on pickup.
	 *   - Generates a unique PlacementID.
	 *   - Calls SetGhostMode(false) to re-enable collision and clear material overrides.
	 */
	void FinalizePlace(UItemDefinition* ItemDef);

	// ── IInteractable ──────────────────────────────────────────────────────

	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;

	/**
	 * Picks up the actor: returns SourceItemDef×1 to the interactor's inventory.
	 * Destroys self on success. Logs a warning if the inventory is full.
	 */
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;
};
