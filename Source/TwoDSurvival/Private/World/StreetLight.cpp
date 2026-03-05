// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetLight.h"
#include "World/TimeManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

AStreetLight::AStreetLight()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
}

void AStreetLight::BeginPlay()
{
	Super::BeginPlay();

	ATimeManager* TM = Cast<ATimeManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ATimeManager::StaticClass()));

	if (TM)
	{
		TM->OnDayPhaseChanged.AddDynamic(this, &AStreetLight::OnDayPhaseChanged);
		SetLightOn(TM->IsNight()); // correct initial state
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetLight] No ATimeManager found — light will stay off."));
	}
}

void AStreetLight::OnDayPhaseChanged(bool bIsNight)
{
	SetLightOn(bIsNight);
}

void AStreetLight::SetLightOn(bool bOn)
{
	Light->SetVisibility(bOn);
	Light->SetIntensity(bOn ? LightIntensity : 0.f);
	Light->SetLightColor(LightColor);
}
