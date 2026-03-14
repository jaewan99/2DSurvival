// Fill out your copyright notice in the Description page of Project Settings.

#include "World/RoomCell.h"
#include "World/RoomDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ARoomCell::ARoomCell()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// ── Floor ────────────────────────────────────────────────────────────────
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(RootComponent);
	Floor->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Ceiling ──────────────────────────────────────────────────────────────
	Ceiling = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ceiling"));
	Ceiling->SetupAttachment(RootComponent);
	Ceiling->SetCollisionProfileName(TEXT("BlockAll"));

	// ── Facade ───────────────────────────────────────────────────────────────
	// No mesh assigned here — user assigns mesh + material in the Blueprint child.
	FacadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacadeMesh"));
	FacadeMesh->SetupAttachment(RootComponent);
	FacadeMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

// ── ABuildingGenerator interface ──────────────────────────────────────────────

void ARoomCell::SetRoomDimensions(float Width, float Height)
{
	Floor->SetRelativeLocation(FVector(Width * 0.5f, 0.f, 0.f));
	Ceiling->SetRelativeLocation(FVector(Width * 0.5f, 0.f, Height));
}

void ARoomCell::ApplyRoomDefinition_Implementation(URoomDefinition* Definition)
{
	// Base is a no-op — override in the Blueprint child for archetype-specific logic.
}

// ── Facade visibility ─────────────────────────────────────────────────────────

void ARoomCell::SetFacadeVisible(bool bVisible, float Duration)
{
	if (!FacadeMesh) return;

	const float Target = bVisible ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(FadeAlpha, Target) && !bFading) return;

	// Show the mesh before fading in so it's ready to lerp.
	if (bVisible)
		FacadeMesh->SetVisibility(true);

	EnsureFacadeMID();

	FadeTarget = Target;
	FadeSpeed  = (Duration > 0.f) ? (1.f / Duration) : 999.f;
	bFading    = true;

	SetActorTickEnabled(true);
}

void ARoomCell::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFading) return;

	FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, FadeTarget, DeltaTime, FadeSpeed);

	if (FacadeMID)
		FacadeMID->SetScalarParameterValue(TEXT("FacadeOpacity"), FadeAlpha);

	if (FMath::IsNearlyEqual(FadeAlpha, FadeTarget, 0.001f))
	{
		FadeAlpha = FadeTarget;
		bFading   = false;
		SetActorTickEnabled(false);

		// Fully hidden — disable the mesh to save draw calls.
		if (FacadeMID)
			FacadeMID->SetScalarParameterValue(TEXT("FacadeOpacity"), FadeAlpha);
		FacadeMesh->SetVisibility(FadeTarget > 0.5f);
	}
}

void ARoomCell::EnsureFacadeMID()
{
	if (FacadeMID || !FacadeMesh || !FacadeMesh->GetMaterial(0)) return;
	FacadeMID = FacadeMesh->CreateDynamicMaterialInstance(0);
}
