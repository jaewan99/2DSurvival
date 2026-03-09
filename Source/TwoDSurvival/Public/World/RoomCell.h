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
 * Provides Floor and Ceiling mesh components that are repositioned at runtime
 * by ABuildingGenerator. All wall meshes (left, right, back) are intentionally
 * omitted — place them directly in the Blueprint child using the viewport.
 *
 * ABuildingGenerator calls SetRoomDimensions() immediately after spawn so the
 * floor and ceiling are positioned correctly for the building's grid.
 *
 * Blueprint child (BP_RoomCell):
 *   - Assign Static Meshes to Floor and Ceiling in the Details panel.
 *   - Add wall meshes directly in the Blueprint viewport as needed.
 *   - Override ApplyRoomDefinition only for custom material logic.
 */
UCLASS()
class TWODSURVIVAL_API ARoomCell : public AActor
{
	GENERATED_BODY()

public:
	ARoomCell();

	// ── Surface meshes ────────────────────────────────────────────────────────
	// Assign Static Meshes in the Blueprint Details panel. Leave empty to hide.

	/** The floor surface the player walks on. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Floor;

	/** The ceiling surface above the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Ceiling;

	// ── Called by ABuildingGenerator ─────────────────────────────────────────

	/**
	 * Repositions Floor and Ceiling to match the building grid.
	 * Called by ABuildingGenerator immediately after this actor is spawned.
	 * Width  = UBuildingDefinition::RoomWidth
	 * Height = UBuildingDefinition::FloorHeight
	 */
	UFUNCTION(BlueprintCallable, Category = "Room")
	void SetRoomDimensions(float Width, float Height);

	/**
	 * Called by ABuildingGenerator to apply archetype-specific overrides.
	 * The C++ base is a no-op — override in Blueprint for custom material logic.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Room")
	void ApplyRoomDefinition(URoomDefinition* Definition);
	virtual void ApplyRoomDefinition_Implementation(URoomDefinition* Definition);
};
