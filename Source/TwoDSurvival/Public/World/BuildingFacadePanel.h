// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingFacadePanel.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * One per building floor — spawned by ABuildingGenerator at the front face of the building.
 * Covers the full width of the floor with a single mesh that is auto-scaled on spawn.
 *
 * ABuildingInteriorVolume tracks which floor the player is on and fades only that panel,
 * so upper/lower floors stay visible while the player's current floor fades out.
 *
 * Blueprint child (BP_BuildingFacade):
 *   - Assign a mesh to FacadeMesh in the Details panel.
 *   - The mesh must use a translucent material with a scalar parameter named "FacadeOpacity".
 *   - MeshBaseExtent should match the mesh's width/height in cm (default 100 for UE's plane).
 */
UCLASS()
class TWODSURVIVAL_API ABuildingFacadePanel : public AActor
{
	GENERATED_BODY()

public:
	ABuildingFacadePanel();

	/** The facade mesh — assign in the Blueprint Details panel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FacadeMesh;

	/**
	 * The unscaled width/height of the assigned mesh in cm.
	 * Default 100 matches UE's built-in plane mesh.
	 * Adjust if using a custom mesh with a different base size.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Facade")
	float MeshBaseExtent = 100.f;

	/** Called by ABuildingGenerator after spawn. Scales the mesh to cover the floor. */
	void InitPanel(int32 InFloorIndex, float Width, float Height);

	/** Fade in (bVisible=true) or out over Duration seconds. */
	void SetFacadeVisible(bool bVisible, float Duration = 0.35f);

	int32 GetFloorIndex() const { return FloorIndex; }

	virtual void Tick(float DeltaTime) override;

private:
	int32 FloorIndex = 0;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FacadeMID;

	bool  bFading    = false;
	float FadeAlpha  = 1.f;
	float FadeTarget = 1.f;
	float FadeSpeed  = 0.f;

	void EnsureMID();
};
