// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingEntrance.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

ABuildingEntrance::ABuildingEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	SetRootComponent(InteractionBox);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 120.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetGenerateOverlapEvents(true);

	Destination = CreateDefaultSubobject<UArrowComponent>(TEXT("Destination"));
	Destination->SetupAttachment(RootComponent);
	Destination->ArrowColor = FColor(0, 200, 255);
	Destination->ArrowSize = 2.f;
	Destination->bIsScreenSizeScaled = true;
	Destination->SetHiddenInGame(true);
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
	if (MidFadeTimer.IsValid() || EndFadeTimer.IsValid()) return;

	PendingInteractor = Interactor;
	Interactor->bMovementLocked = true;

	const float HalfDuration = FadeTransitionDuration * 0.5f;
	if (APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PCM->StartCameraFade(0.f, 1.f, HalfDuration, FLinearColor::Black, false, true);
	}

	GetWorldTimerManager().SetTimer(MidFadeTimer, this, &ABuildingEntrance::OnMidFade, HalfDuration, false);
}

void ABuildingEntrance::OnMidFade()
{
	ABaseCharacter* Interactor = PendingInteractor.Get();
	if (!Interactor) return;

	Interactor->SetActorLocation(Destination->GetComponentLocation());

	const float HalfDuration = FadeTransitionDuration * 0.5f;
	if (APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		PCM->StartCameraFade(1.f, 0.f, HalfDuration, FLinearColor::Black, false, false);
	}

	GetWorldTimerManager().SetTimer(EndFadeTimer, this, &ABuildingEntrance::OnFadeComplete, HalfDuration, false);
}

void ABuildingEntrance::OnFadeComplete()
{
	if (ABaseCharacter* Interactor = PendingInteractor.Get())
	{
		Interactor->bMovementLocked = false;
	}
	PendingInteractor.Reset();
	MidFadeTimer.Invalidate();
	EndFadeTimer.Invalidate();
}
