// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetBootstrapper.h"
#include "World/StreetDefinition.h"
#include "World/StreetManager.h"
#include "Engine/GameInstance.h"

void AStreetBootstrapper::BeginPlay()
{
	Super::BeginPlay();

	if (!StartingStreet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetBootstrapper] No StartingStreet assigned — nothing will load."));
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			SM->InitializeWithStreet(StartingStreet, StartingOffset);
		}
	}
}
