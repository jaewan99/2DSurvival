// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomCell.generated.h"

class URoomDefinition;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * C++ base class for all room cell Blueprint actors.
 *
 * Provides Floor, Ceiling, and FacadeMesh components. Floor and Ceiling are
 * repositioned at runtime by ABuildingGenerator. All interior wall meshes
 * (left, right, back) are placed in the Blueprint child viewport.
 *
 * FacadeMesh is the front-facing exterior wall visible from the street.
 * Assign a mesh + translucent material with a "FacadeOpacity" scalar parameter
 * in the Blueprint Details panel. ABuildingInteriorVolume fades it in/out when
 * the player enters or exits the building.
 *
 * Blueprint child (BP_RoomCell):
 *   - Assign Static Meshes to Floor, Ceiling, FacadeMesh in the Details panel.
 *   - FacadeMesh material must expose a "FacadeOpacity" scalar (0=hidden, 1=visible).
 *   - Add interior wall meshes in the Blueprint viewport as needed.
 */
UCLASS()
class TWODSURVIVAL_API ARoomCell : public AActor
{
	GENERATED_BODY()

public:
	ARoomCell();

	// ── Surface meshes ────────────────────────────────────────────────────────

	/** The floor surface the player walks on. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Floor;

	/** The ceiling surface above the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Meshes")
	TObjectPtr<UStaticMeshComponent> Ceiling;

	/**
	 * The exterior facade panel visible from the street.
	 * Assign a mesh + translucent material with "FacadeOpacity" scalar in the BP Details panel.
	 * ABuildingInteriorVolume calls SetFacadeVisible() to fade this in/out.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room|Facade")
	TObjectPtr<UStaticMeshComponent> FacadeMesh;

	// ── Called by ABuildingGenerator ─────────────────────────────────────────

	/** Repositions Floor and Ceiling to match the building grid. */
	UFUNCTION(BlueprintCallable, Category = "Room")
	void SetRoomDimensions(float Width, float Height);

	/** Called by ABuildingGenerator to apply archetype-specific overrides. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Room")
	void ApplyRoomDefinition(URoomDefinition* Definition);
	virtual void ApplyRoomDefinition_Implementation(URoomDefinition* Definition);

	// ── Facade visibility ─────────────────────────────────────────────────────

	/**
	 * Fade the facade in or out over Duration seconds.
	 * Drives the "FacadeOpacity" scalar parameter on a dynamic material instance.
	 * If the material does not expose that parameter the mesh is still shown/hidden
	 * instantly at the end of the fade as a fallback.
	 */
	void SetFacadeVisible(bool bVisible, float Duration = 0.35f);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FacadeMID;

	bool  bFading     = false;
	float FadeAlpha   = 1.f;   // current opacity (0–1)
	float FadeTarget  = 1.f;
	float FadeSpeed   = 0.f;

	void EnsureFacadeMID();
};
