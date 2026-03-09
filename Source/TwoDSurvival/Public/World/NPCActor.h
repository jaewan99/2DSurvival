// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "NPCActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UNPCDefinition;
class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNPCTradeCompleted, ANPCActor*, NPC);

/**
 * An NPC actor placed in the world.
 * Press E to open the dialogue widget. Walking out of range auto-closes it.
 * Blueprint child (BP_NPCActor) — assign mesh and NPCDef in the Details panel.
 *
 * OnTradeCompleted — BlueprintAssignable delegate fired when the player gives the
 * required item. Bind in Blueprint to unlock doors, spawn loot, etc.
 */
UCLASS()
class TWODSURVIVAL_API ANPCActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ANPCActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	// Assign a UNPCDefinition data asset in the Blueprint child's Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UNPCDefinition> NPCDef;

	// Fired when the player successfully completes the trade with this NPC.
	// Bind in Blueprint level to unlock paths, open doors, etc.
	UPROPERTY(BlueprintAssignable, Category = "NPC")
	FOnNPCTradeCompleted OnTradeCompleted;

	// True after the trade is completed (set by the dialogue widget, persisted via SaveGame).
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	bool bTradeCompleted = false;

	// Called by UDialogueWidget after a successful trade — fires OnTradeCompleted delegate.
	void NotifyTradeCompleted();

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override;
	virtual float GetInteractionDuration_Implementation() override;
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<ABaseCharacter> InteractingPlayer;

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
