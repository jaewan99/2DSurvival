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
 * Creates five mesh components out of the box — assign Static Meshes in the
 * Blueprint Details panel. Leave any component's mesh empty to hide that surface.
 *
 * ABuildingGenerator calls SetRoomDimensions() immediately after spawn so the
 * walls / floor / ceiling are positioned correctly for the building's grid.
 * It then calls ApplyRoomDefinition() to apply wall and floor materials.
 *
 * Blueprint child (BP_RoomCell):
 *   - Assign Static Meshes to Floor, Ceiling, LeftWall, RightWall, BackWall.
 *   - Override ApplyRoomDefinition only if you need custom material logic beyond
 *     the defaults (C++ base already handles WallMaterial and FloorMaterial).
 */
UCLASS()
class TWODSURVIVAL_API ARoomCell : public AActor
{
	GENERATED_BODY()

public:
	ARoomCell();

	// ── Wall / surface meshes ─────────────────────────────────────────────────
	// Assign Static Meshes in the Blueprint Details panel. Any left empty is invisible.

	// The floor surface the player walks on.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Floor;

	// The ceiling surface above the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Ceiling;

	// Left boundary wall (at X = 0 relative to room origin).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> LeftWall;

	// Right boundary wall (at X = RoomWidth relative to room origin).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> RightWall;

	// Background wall visible behind the characters (depth / backdrop).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> BackWall;

	// ── Called by ABuildingGenerator ─────────────────────────────────────────

	/**
	 * Repositions all wall/floor/ceiling components to match the building grid.
	 * Called by ABuildingGenerator immediately after this actor is spawned.
	 * Width  = UBuildingDefinition::RoomWidth
	 * Height = UBuildingDefinition::FloorHeight
	 */
	UFUNCTION(BlueprintCallable, Category = "Room")
	void SetRoomDimensions(float Width, float Height);

	/**
	 * Applies WallMaterial (LeftWall, RightWall, BackWall) and FloorMaterial (Floor)
	 * from the given definition. The C++ base already does this — override in Blueprint
	 * only for custom material logic (e.g. setting additional material parameters).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Room")
	void ApplyRoomDefinition(URoomDefinition* Definition);
	virtual void ApplyRoomDefinition_Implementation(URoomDefinition* Definition);
};
