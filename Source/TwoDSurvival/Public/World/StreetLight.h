// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetLight.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A street light that turns on at night and off during the day.
 * Binds to ATimeManager::OnDayPhaseChanged at BeginPlay — no manual wiring needed.
 * Blueprint child (BP_StreetLight) — assign a mesh and position the Light component.
 */
UCLASS()
class TWODSURVIVAL_API AStreetLight : public AActor
{
	GENERATED_BODY()

public:
	AStreetLight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Light;

	// Intensity when the light is on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float LightIntensity = 2000.f;

	// Warm yellow by default — tweak per light in the Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	FLinearColor LightColor = FLinearColor(1.f, 0.85f, 0.5f);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnDayPhaseChanged(bool bIsNight);

	void SetLightOn(bool bOn);
};
