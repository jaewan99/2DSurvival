// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingEntrance.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
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

	// Guard against double-triggering while a transition is already running.
	if (MidFadeTimer.IsValid() || EndFadeTimer.IsValid()) return;

	PendingInteractor = Interactor;
	bPendingEntering = !Interactor->bIsInsideDepthBuilding;
	PendingTargetY = InteriorY;

	// Lock movement for the duration of the transition.
	Interactor->bMovementLocked = true;

	// Fade to black over the first half of the transition duration.
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

	// Do the teleport while the screen is fully black.
	if (bPendingEntering)
	{
		Interactor->EnterDepthLayer(PendingTargetY);
	}
	else
	{
		Interactor->ExitDepthLayer();
	}

	// Fade back in over the second half.
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
}
