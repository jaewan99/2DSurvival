// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingInteriorVolume.generated.h"

class UBoxComponent;
class ABuildingFacadePanel;

/**
 * Invisible trigger volume spawned by ABuildingGenerator after Generate().
 * Sized to cover all building interior floors — intentionally stops BELOW the
 * rooftop so players standing on the roof are NOT considered "inside".
 *
 * Fades the building's single facade panel out when the player enters and
 * back in when they leave. No per-floor logic — simple enter/exit toggle.
 *
 * This is purely runtime — no Blueprint child needed.
 */
UCLASS()
class TWODSURVIVAL_API ABuildingInteriorVolume : public AActor
{
	GENERATED_BODY()

public:
	ABuildingInteriorVolume();

	/** Exposed so ABuildingGenerator can resize the box after spawn. */
	UBoxComponent* GetTriggerBox() const { return TriggerBox; }

	/** Called by ABuildingGenerator after spawn. May be null if no FacadePanelClass was set. */
	void SetFacadePanel(ABuildingFacadePanel* Panel);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY()
	TObjectPtr<ABuildingFacadePanel> FacadePanel;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
