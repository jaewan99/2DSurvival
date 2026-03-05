// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingEntrance.h"
#include "World/StreetManager.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"

ABuildingEntrance::ABuildingEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(InteractionBox);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 120.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetGenerateOverlapEvents(true);
}

FText ABuildingEntrance::GetInteractionPrompt_Implementation()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			return SM->bIsInsideBuilding
				? FText::FromString(TEXT("Exit Building"))
				: FText::FromString(TEXT("Enter Building"));
		}
	}
	return FText::FromString(TEXT("Enter"));
}

void ABuildingEntrance::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			SM->OnPlayerCrossedExit(EExitDirection::Up);
		}
	}
}
