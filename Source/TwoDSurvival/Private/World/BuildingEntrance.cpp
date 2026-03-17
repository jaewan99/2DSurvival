// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingEntrance.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ABuildingEntrance::ABuildingEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(InteractionBox);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 120.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetGenerateOverlapEvents(true);
}

void ABuildingEntrance::BeginPlay()
{
	Super::BeginPlay();

	// Extend the box Y to cover both the street layer and the interior layer so the player's
	// detection sphere (radius 150) can reach it from either side.
	const FVector Ext = InteractionBox->GetUnscaledBoxExtent();
	InteractionBox->SetBoxExtent(FVector(Ext.X, InteriorY + 80.f, Ext.Z));
}

FText ABuildingEntrance::GetInteractionPrompt_Implementation()
{
	if (ABaseCharacter* BC = Cast<ABaseCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		return BC->bIsInsideDepthBuilding
			? FText::FromString(TEXT("Exit Building"))
			: FText::FromString(TEXT("Enter Building"));
	}
	return FText::FromString(TEXT("Enter Building"));
}

void ABuildingEntrance::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor) return;

	if (Interactor->bIsInsideDepthBuilding)
	{
		Interactor->ExitDepthLayer();
	}
	else
	{
		Interactor->EnterDepthLayer(InteriorY);
	}
}
