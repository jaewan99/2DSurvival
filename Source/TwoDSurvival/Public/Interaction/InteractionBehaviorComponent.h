// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Interaction/PropInteractionTypes.h"
#include "InteractionBehaviorComponent.generated.h"

class ABaseCharacter;

/**
 * Abstract base for all prop interaction behaviors.
 * Add one or more subclass components to an AWorldProp.
 * The highest-priority available component drives the interaction prompt and E-key response.
 *
 * Subclasses:
 *   UBreakableComponent   — weapon damage + health + loot on death
 *   UDisassembleComponent — E-key instant/hold loot
 *   (add more as needed — one new component file per new behavior)
 */
UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UInteractionBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Higher number wins when multiple components are on the same actor.
	 * Default priorities: BreakableComponent=10, DisassembleComponent=5.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction")
	int32 Priority = 0;

	/** Tool required to execute this behavior. None = any weapon or bare hands. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction")
	EToolType RequiredTool = EToolType::None;

	/**
	 * Short name shown in the interaction prompt (e.g. "Wooden Planks", "Old Table").
	 * Set this in the Blueprint child's Details panel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText ActionLabel;

	/** Text shown in the interaction HUD (e.g. "Break [Planks] (Hammer)"). */
	virtual FText GetPrompt() const;

	/** Return false to deactivate this behavior permanently (e.g. already broken/used). */
	virtual bool IsAvailable() const;

	/**
	 * Return false if the interactor doesn't meet requirements.
	 * Base implementation checks RequiredTool against the interactor's equipped weapon.
	 */
	virtual bool CanInteract(ABaseCharacter* Interactor) const;

	/** Perform the interaction — called by AWorldProp when the player presses E. */
	virtual void Execute(ABaseCharacter* Interactor);

	/** Interaction type (Instant or Hold). Subclasses override for hold interactions. */
	virtual EInteractionType GetInteractionType() const;

	/** Hold duration in seconds — only relevant when GetInteractionType() == Hold. */
	virtual float GetInteractionDuration() const;

protected:
	/** Returns the ToolType of the interactor's equipped weapon. None if unarmed. */
	EToolType GetEquippedToolType(ABaseCharacter* Interactor) const;
};
