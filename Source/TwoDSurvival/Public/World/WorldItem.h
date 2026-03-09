// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "WorldItem.generated.h"

class UItemDefinition;
class UStaticMeshComponent;
class UBoxComponent;
class USoundBase;

/**
 * A physical item dropped in the world (e.g. enemy loot).
 * Implements IInteractable — the player's UInteractionComponent detects it automatically.
 * Press E to pick up: item goes into the player's inventory, then the actor self-destructs.
 *
 * Spawned by AEnemyBase::SpawnLoot(). After spawning, set ItemDef and Quantity.
 */
UCLASS()
class TWODSURVIVAL_API AWorldItem : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AWorldItem();

	// Placeholder visual mesh. Swap for a proper prop in BP_WorldItem if desired.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Box used by UInteractionComponent for proximity detection.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UBoxComponent> InteractionBox;

	// Set by EnemyBase::SpawnLoot() after the actor is spawned.
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	TObjectPtr<UItemDefinition> ItemDef;

	// Number of items this pickup contains.
	UPROPERTY(BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_Pickup;

protected:
	virtual void BeginPlay() override;

public:
	// IInteractable interface
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;

	// Adds the item to the interactor's inventory and destroys this actor.
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;
};
