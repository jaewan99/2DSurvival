// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingInteriorVolume.generated.h"

class UBoxComponent;
class ABuildingGenerator;

/**
 * Invisible trigger volume spawned by ABuildingGenerator after Generate().
 * Sized to cover all building interior floors — intentionally stops BELOW the
 * rooftop so players standing on the roof are NOT considered "inside".
 *
 * When the player overlaps this volume the owning generator fades all room
 * cell facades out. When they leave, facades fade back in.
 *
 * This is purely runtime — no Blueprint child needed.
 */
UCLASS()
class TWODSURVIVAL_API ABuildingInteriorVolume : public AActor
{
	GENERATED_BODY()

public:
	ABuildingInteriorVolume();

	/** Called by ABuildingGenerator after spawn to wire up the generator reference. */
	void SetOwningGenerator(ABuildingGenerator* Generator);

	/** Exposed so ABuildingGenerator can resize the box after spawn. */
	UBoxComponent* GetTriggerBox() const { return TriggerBox; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY()
	TObjectPtr<ABuildingGenerator> OwningGenerator;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
