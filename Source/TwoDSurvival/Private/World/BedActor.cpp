// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BedActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Character/BaseCharacter.h"

ABedActor::ABedActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(120.f, 120.f, 80.f));  // Wide enough to stand beside the bed
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void ABedActor::BeginPlay()
{
	Super::BeginPlay();

	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ABedActor::OnBoxEndOverlap);
}

EInteractionType ABedActor::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float ABedActor::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText ABedActor::GetInteractionPrompt_Implementation()
{
	const bool bActuallySleeping = SleepingPlayer.IsValid()
		&& SleepingPlayer->NeedsComponent
		&& SleepingPlayer->NeedsComponent->bIsSleeping;

	return bActuallySleeping
		? FText::FromString(TEXT("Wake Up"))
		: FText::FromString(TEXT("Sleep"));
}

void ABedActor::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	// Check bIsSleeping on the component — after auto-wake, SleepingPlayer may still be set
	// but the character is already awake. Treat that as "no one sleeping".
	const bool bActuallySleeping = SleepingPlayer.IsValid()
		&& SleepingPlayer->NeedsComponent
		&& SleepingPlayer->NeedsComponent->bIsSleeping;

	if (bActuallySleeping)
	{
		SleepingPlayer->StopSleeping();
		SleepingPlayer = nullptr;
	}
	else
	{
		// Clear any stale reference before starting a new sleep session
		SleepingPlayer = Interactor;
		Interactor->StartSleeping();
	}
}

void ABedActor::OnBoxEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	// Clear the stale reference if the player leaves — but do NOT auto-wake.
	// SetMovementMode(MOVE_None) fires a false EndOverlap as a physics artifact,
	// so we can't reliably distinguish walking away from a physics event.
	// Wake-up is handled exclusively by: pressing E, or Fatigue reaching 100.
	if (SleepingPlayer.IsValid() && OtherActor == SleepingPlayer.Get()
		&& !(SleepingPlayer->NeedsComponent && SleepingPlayer->NeedsComponent->bIsSleeping))
	{
		SleepingPlayer = nullptr;
	}
}
