// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingFacadePanel.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ABuildingFacadePanel::ABuildingFacadePanel()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	FacadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacadeMesh"));
	SetRootComponent(FacadeMesh);
	FacadeMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void ABuildingFacadePanel::InitPanel(int32 InFloorIndex, float Width, float Height)
{
	FloorIndex = InFloorIndex;

	// Scale the mesh so it covers the full floor width and height.
	// MeshBaseExtent is the mesh's unscaled size in cm (100 = UE's default plane mesh).
	const float Extent = FMath::Max(MeshBaseExtent, 1.f);
	FacadeMesh->SetRelativeScale3D(FVector(Width / Extent, 1.f, Height / Extent));

	// Offset up so the actor's origin sits at the floor base and the mesh centers on mid-floor.
	FacadeMesh->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));
}

void ABuildingFacadePanel::SetFacadeVisible(bool bVisible, float Duration)
{
	EnsureMID();

	const float Target = bVisible ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(FadeAlpha, Target) && !bFading) return;

	if (bVisible) FacadeMesh->SetVisibility(true);

	FadeTarget = Target;
	FadeSpeed  = (Duration > 0.f) ? (1.f / Duration) : 999.f;
	bFading    = true;
	SetActorTickEnabled(true);
}

void ABuildingFacadePanel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bFading) return;

	FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, FadeTarget, DeltaTime, FadeSpeed);
	if (FacadeMID) FacadeMID->SetScalarParameterValue(TEXT("FacadeOpacity"), FadeAlpha);

	if (FMath::IsNearlyEqual(FadeAlpha, FadeTarget, 0.001f))
	{
		FadeAlpha = FadeTarget;
		bFading   = false;
		SetActorTickEnabled(false);

		if (FacadeMID) FacadeMID->SetScalarParameterValue(TEXT("FacadeOpacity"), FadeAlpha);
		FacadeMesh->SetVisibility(FadeTarget > 0.5f);
	}
}

void ABuildingFacadePanel::EnsureMID()
{
	if (FacadeMID) return;
	if (FacadeMesh && FacadeMesh->GetMaterial(0))
		FacadeMID = FacadeMesh->CreateDynamicMaterialInstance(0);
}
