// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCell.generated.h"

class URoomDefinition;
class UStaticMeshComponent;

/**
 * C++ base class for all room cell Blueprint actors.
 *
 * Floor is the only C++ component — auto-centered by SetRoomDimensions().
 * All other geometry (walls, furniture, props) is placed freely in the
 * Blueprint child viewport.
 *
 * Facade fading is handled by ABuildingFacadePanel — one per floor, spawned
 * by ABuildingGenerator. Room cells no longer need any facade tagging.
 *
 * Blueprint child (BP_RoomCell_*):
 *   - Assign a mesh to Floor in the Details panel.
 *   - Add wall, furniture, and prop meshes freely in the viewport.
 */
UCLASS()
class TWODSURVIVAL_API ARoomCell : public AActor
{
	GENERATED_BODY()

public:
	ARoomCell();

	/**
	 * The floor surface for this room level.
	 * Also acts as the ceiling for the room below — no separate ceiling needed.
	 * Assign a mesh in the Blueprint Details panel.
	 * SetRoomDimensions() centers it on the room width at runtime.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Floor;

	// ── Called by ABuildingGenerator ─────────────────────────────────────────

	/**
	 * Centers the Floor mesh horizontally on the room width.
	 * Called by ABuildingGenerator after spawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Room")
	void SetRoomDimensions(float Width);

	/** Called by ABuildingGenerator to apply archetype-specific overrides. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Room")
	void ApplyRoomDefinition(URoomDefinition* Definition);
	virtual void ApplyRoomDefinition_Implementation(URoomDefinition* Definition);
};
