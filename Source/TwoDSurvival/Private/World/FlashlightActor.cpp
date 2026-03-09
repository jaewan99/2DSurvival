// Fill out your copyright notice in the Description page of Project Settings.

#include "World/FlashlightActor.h"
#include "Components/SpotLightComponent.h"

AFlashlightActor::AFlashlightActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SetRootComponent(SpotLight);

	// Default cone: narrow beam, moderate reach — tune in BP_FlashlightActor.
	SpotLight->InnerConeAngle       = 12.f;
	SpotLight->OuterConeAngle       = 25.f;
	SpotLight->AttenuationRadius    = 1200.f;
	SpotLight->Intensity            = 8000.f;
	SpotLight->SetVisibility(true);
}

void AFlashlightActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLightOn) return;

	BatteryCharge -= DrainRate * DeltaTime;
	if (BatteryCharge <= 0.f)
	{
		BatteryCharge = 0.f;
		bIsLightOn = false;
		SpotLight->SetVisibility(false);
		UE_LOG(LogTemp, Warning, TEXT("[FlashlightActor] Battery depleted."));
	}
}

void AFlashlightActor::Toggle()
{
	if (!bIsLightOn && BatteryCharge <= 0.f) return; // dead battery — can't turn on

	bIsLightOn = !bIsLightOn;
	SpotLight->SetVisibility(bIsLightOn);
}

void AFlashlightActor::RefillBattery(float Amount)
{
	BatteryCharge = FMath::Clamp(BatteryCharge + Amount, 0.f, 100.f);
	UE_LOG(LogTemp, Log, TEXT("[FlashlightActor] Battery refilled to %.1f"), BatteryCharge);
}

bool AFlashlightActor::IsInCone(FVector WorldPos) const
{
	if (!bIsLightOn || !SpotLight) return false;

	const FVector ToTarget    = (WorldPos - GetActorLocation()).GetSafeNormal();
	const FVector LightFwd    = SpotLight->GetForwardVector();
	const float   Dot         = FVector::DotProduct(LightFwd, ToTarget);
	const float   HalfAngle   = FMath::DegreesToRadians(SpotLight->OuterConeAngle);

	return Dot >= FMath::Cos(HalfAngle);
}
