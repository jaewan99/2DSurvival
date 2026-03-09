// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetExit.generated.h"

class UBoxComponent;

/**
 * Walk-through trigger placed inside a street sublevel.
 * When the player enters the TriggerBox, UStreetManager streams in the adjacent level
 * defined by the FStreetExitLink whose ExitID matches this actor's ExitID.
 *
 * Blueprint child (BP_StreetExit):
 *   - Set ExitID to match the exit in DA_Street_* (e.g. "Left", "Right", "BackAlley")
 *   - Resize TriggerBox to cover the full height of the exit corridor
 *   - Optionally add a mesh as a visual marker
 *
 * Placement tip: put the trigger 200–400 units INSIDE the current street's end so
 * the next level has time to load before the player physically crosses into it.
 *
 * For press-E building entrances, use ABuildingEntrance instead.
 */
UCLASS()
class TWODSURVIVAL_API AStreetExit : public AActor
{
	GENERATED_BODY()

public:
	AStreetExit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/**
	 * Must match a FStreetExitLink::ExitID on the current street's UStreetDefinition.
	 * e.g. "Left", "Right", "BackAlley", "RoofHatch" — any name is valid.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FName ExitID = NAME_None;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
