// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionBehaviorComponent.h"
#include "Enemy/EnemyTypes.h"
#include "DisassembleComponent.generated.h"

class USoundBase;

/**
 * Gives an AWorldProp an E-key disassembly action that yields items and destroys the actor.
 *
 * Set HoldDuration > 0 for a hold interaction (e.g. 2s to disassemble a table).
 * Set HoldDuration = 0 for an instant press (e.g. pick apart some loose boards).
 *
 * Default Priority = 5 (lower than BreakableComponent's 10, so if both are on the same actor
 * the BreakableComponent takes over until it's destroyed, then Disassemble becomes available).
 *
 * Blueprint steps: add DisassembleComponent to a BP_WorldProp child, set ActionLabel,
 * HoldDuration, and Yield loot entries in the Details panel.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UDisassembleComponent : public UInteractionBehaviorComponent
{
	GENERATED_BODY()

public:
	UDisassembleComponent();

	/** Items yielded when the player successfully disassembles this prop. */
	UPROPERTY(EditDefaultsOnly, Category="Disassemble")
	TArray<FLootEntry> Yield;

	/** Hold duration in seconds. 0 = instant E-press. */
	UPROPERTY(EditDefaultsOnly, Category="Disassemble", meta=(ClampMin="0.0"))
	float HoldDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	TObjectPtr<USoundBase> SFX_Disassemble;

	// UInteractionBehaviorComponent
	virtual FText GetPrompt() const override;
	virtual void Execute(ABaseCharacter* Interactor) override;
	virtual EInteractionType GetInteractionType() const override;
	virtual float GetInteractionDuration() const override;

private:
	void SpawnYield();
};
