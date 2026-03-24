// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingInteriorVolume.generated.h"

class UBoxComponent;
class ABuildingFacadePanel;
class ABaseCharacter;

/**
 * Invisible trigger volume spawned by ABuildingGenerator after Generate().
 * Sized to cover all building interior floors — intentionally stops BELOW the
 * rooftop so players standing on the roof are NOT considered "inside".
 *
 * Per-floor facade fading:
 *   ABuildingGenerator calls SetFacadePanels() after spawn, passing one
 *   ABuildingFacadePanel per floor. The volume ticks while the player is
 *   inside, detects floor changes via Z position, and fades only the
 *   current floor's panel out — other floors stay visible.
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

	/**
	 * Register facade panels and floor metrics.
	 * Panels must be in ascending floor order (index 0 = ground floor).
	 * Called by ABuildingGenerator immediately after spawn.
	 */
	void SetFacadePanels(const TArray<ABuildingFacadePanel*>& Panels,
	                     float InBuildingBaseZ, float InFloorHeight);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerBox;

	// One entry per floor — may be empty if no FacadePanelClass was set on the generator.
	UPROPERTY()
	TArray<TObjectPtr<ABuildingFacadePanel>> FacadePanels;

	float BuildingBaseZ = 0.f;
	float FloorHeight   = 300.f;
	int32 ActiveFloor   = -1;

	TWeakObjectPtr<ABaseCharacter> TrackedPlayer;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Recalculates the player's current floor and fades the right panel. */
	void UpdateFacadeForPlayer();
};
