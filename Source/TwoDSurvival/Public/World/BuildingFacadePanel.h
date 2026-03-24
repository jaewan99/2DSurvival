// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingFacadePanel.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Single facade actor for the entire building front — spawned by ABuildingGenerator,
 * auto-scaled to cover the full building width and total height.
 *
 * ABuildingInteriorVolume fades it out when the player enters and back in on exit.
 *
 * Blueprint child (BP_BuildingFacade):
 *   - Assign a mesh to FacadeMesh in the Details panel.
 *   - Material must be translucent with a scalar parameter named "FacadeOpacity".
 *   - Set MeshBaseExtent to match the mesh's base size in cm (default 100 = UE plane).
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
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Facade")
	float MeshBaseExtent = 100.f;

	/** Called by ABuildingGenerator after spawn. Scales the mesh to cover the full building. */
	void InitPanel(float Width, float Height);

	/** Fade in (bVisible=true) or out over Duration seconds. */
	void SetFacadeVisible(bool bVisible, float Duration = 0.35f);

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FacadeMID;

	bool  bFading    = false;
	float FadeAlpha  = 1.f;
	float FadeTarget = 1.f;
	float FadeSpeed  = 0.f;

	void EnsureMID();
};
