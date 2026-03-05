// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetExit.h"
#include "World/StreetManager.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"

AStreetExit::AStreetExit()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	// Default: thin on X (the crossing axis), tall/wide on Y/Z to cover the corridor.
	// Resize in the Blueprint child to fit each street's exit geometry.
	TriggerBox->SetBoxExtent(FVector(10.f, 200.f, 400.f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AStreetExit::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AStreetExit::OnTriggerOverlap);
}

void AStreetExit::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only the player triggers a street transition
	if (!Cast<ABaseCharacter>(OtherActor)) return;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			SM->OnPlayerCrossedExit(Direction);
		}
	}
}
