// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BedActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ABaseCharacter;

/**
 * A bed that the player can interact with to sleep and restore Fatigue.
 * Press E to start sleeping (movement locked, Fatigue restores at 3× speed).
 * Press E again, or walk out of range, to wake up.
 * Create a Blueprint child (BP_BedActor) and assign a static mesh.
 */
UCLASS()
class TWODSURVIVAL_API ABedActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABedActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

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
	// The player currently sleeping in this bed. Null when the bed is empty.
	TWeakObjectPtr<ABaseCharacter> SleepingPlayer;

	UFUNCTION()
	void OnBoxEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};
