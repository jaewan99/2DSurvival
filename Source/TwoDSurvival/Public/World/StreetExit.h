// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/StreetDefinition.h"
#include "StreetExit.generated.h"

class UBoxComponent;

/**
 * Placed at the Left or Right boundary of a street sublevel.
 * When the player walks through the trigger box, UStreetManager streams in the adjacent street
 * and unloads the current one — no E-press required.
 *
 * Blueprint child (BP_StreetExit):
 *   - Set Direction in Details (Left or Right)
 *   - Resize TriggerBox to cover the full height of the exit corridor
 *   - Optionally add a mesh (e.g. street-end barrier) as a visual marker
 *
 * Up exits are NOT handled here — use a separate interactable actor (door/ladder).
 * Exit connectivity is defined on UStreetDefinition (ExitLeft/ExitRight), not here.
 */
UCLASS()
class TWODSURVIVAL_API AStreetExit : public AActor
{
	GENERATED_BODY()

public:
	AStreetExit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	// Which boundary of this street this actor represents.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	EExitDirection Direction = EExitDirection::Right;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
