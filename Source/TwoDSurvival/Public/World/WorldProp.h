// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "Combat/DamageableInterface.h"
#include "WorldProp.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UInteractionBehaviorComponent;

/**
 * Generic world prop that delegates interaction and damage to behavior components.
 *
 * Add any combination of UInteractionBehaviorComponent subclasses:
 *   - UBreakableComponent   — hit with the right weapon to destroy + drop loot
 *   - UDisassembleComponent — press/hold E to disassemble + yield items
 *   - (future components added here without changing this class)
 *
 * The highest-priority available component drives the interaction prompt and E-key response.
 * When a weapon hitbox overlaps this actor, TakeMeleeDamage routes to UBreakableComponent.
 *
 * Blueprint steps (per prop type):
 *   1. Create a child BP (e.g. BP_WoodPlanks) of AWorldProp.
 *   2. Assign a mesh in the Details panel.
 *   3. Add behavior component(s) and fill their settings (ActionLabel, LootTable, etc.).
 */
UCLASS()
class TWODSURVIVAL_API AWorldProp : public AActor, public IInteractable, public IDamageable
{
	GENERATED_BODY()

public:
	AWorldProp();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prop")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Proximity trigger used by UInteractionComponent to detect this prop. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prop")
	TObjectPtr<UBoxComponent> InteractionBox;

	// IInteractable — all four delegate to the active behavior component
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

	// IDamageable — routes to UBreakableComponent if present
	virtual void TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource) override;

private:
	/** Returns the highest-priority available behavior component, or null if none. */
	UInteractionBehaviorComponent* GetActiveBehavior() const;
};
