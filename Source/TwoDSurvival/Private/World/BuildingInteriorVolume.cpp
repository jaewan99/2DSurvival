// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingInteriorVolume.h"
#include "World/BuildingFacadePanel.h"
#include "Character/BaseCharacter.h"
#include "Components/NeedsComponent.h"
#include "Components/BoxComponent.h"

ABuildingInteriorVolume::ABuildingInteriorVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(400.f, 300.f, 400.f)); // resized by generator after spawn
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABuildingInteriorVolume::OnBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ABuildingInteriorVolume::OnEndOverlap);
}

void ABuildingInteriorVolume::SetFacadePanels(const TArray<ABuildingFacadePanel*>& Panels,
                                               float InBuildingBaseZ, float InFloorHeight)
{
	FacadePanels.Reset();
	for (ABuildingFacadePanel* P : Panels)
		FacadePanels.Add(P);

	BuildingBaseZ = InBuildingBaseZ;
	FloorHeight   = FMath::Max(InFloorHeight, 1.f);
}

void ABuildingInteriorVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (!Player) return;

	Player->NeedsComponent->bIsIndoors = true;
	Player->bIsInsideDepthBuilding = true;

	if (FacadePanels.Num() > 0)
	{
		TrackedPlayer = Player;
		ActiveFloor   = -1; // force UpdateFacadeForPlayer to apply on first call
		UpdateFacadeForPlayer();
		SetActorTickEnabled(true);
	}
}

void ABuildingInteriorVolume::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (!Player) return;

	Player->NeedsComponent->bIsIndoors = false;
	Player->bIsInsideDepthBuilding = false;

	// Fade all panels back in.
	for (ABuildingFacadePanel* Panel : FacadePanels)
		if (Panel) Panel->SetFacadeVisible(true, 0.35f);

	TrackedPlayer = nullptr;
	ActiveFloor   = -1;
	SetActorTickEnabled(false);
}

void ABuildingInteriorVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFacadeForPlayer();
}

void ABuildingInteriorVolume::UpdateFacadeForPlayer()
{
	if (!TrackedPlayer.IsValid() || FacadePanels.Num() == 0) return;

	const float RelZ     = TrackedPlayer->GetActorLocation().Z - BuildingBaseZ;
	const int32 NewFloor = FMath::Clamp(FMath::FloorToInt(RelZ / FloorHeight),
	                                     0, FacadePanels.Num() - 1);

	if (NewFloor == ActiveFloor) return;

	// Fade previous floor back in.
	if (ActiveFloor >= 0 && FacadePanels.IsValidIndex(ActiveFloor) && FacadePanels[ActiveFloor])
		FacadePanels[ActiveFloor]->SetFacadeVisible(true, 0.35f);

	// Fade new floor out.
	if (FacadePanels.IsValidIndex(NewFloor) && FacadePanels[NewFloor])
		FacadePanels[NewFloor]->SetFacadeVisible(false, 0.35f);

	ActiveFloor = NewFloor;
}
