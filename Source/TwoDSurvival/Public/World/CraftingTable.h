// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "CraftingTable.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ABaseCharacter;

/**
 * A placeable crafting table.
 * Press E to open the crafting UI. Walking away auto-closes it.
 * Blueprint child (BP_CraftingTable) — assign a mesh in the Details panel.
 */
UCLASS()
class TWODSURVIVAL_API ACraftingTable : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ACraftingTable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Box overlap — triggers the player's InteractionComponent detection
	// regardless of the assigned mesh's collision settings.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	// Weak ref to the player currently using this table — cleared on leave.
	TWeakObjectPtr<ABaseCharacter> InteractingPlayer;

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
