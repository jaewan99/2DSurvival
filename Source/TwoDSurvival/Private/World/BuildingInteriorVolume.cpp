// Fill out your copyright notice in the Description page of Project Settings.

#include "World/BuildingInteriorVolume.h"
#include "World/BuildingGenerator.h"
#include "Character/BaseCharacter.h"
#include "Components/NeedsComponent.h"
#include "Components/BoxComponent.h"

ABuildingInteriorVolume::ABuildingInteriorVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(400.f, 300.f, 400.f)); // resized by generator after spawn
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABuildingInteriorVolume::OnBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ABuildingInteriorVolume::OnEndOverlap);
}

void ABuildingInteriorVolume::SetOwningGenerator(ABuildingGenerator* Generator)
{
	OwningGenerator = Generator;
}

void ABuildingInteriorVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (!Player) return;

	Player->NeedsComponent->bIsIndoors = true;
	Player->bIsInsideDepthBuilding = true;
	if (OwningGenerator) OwningGenerator->SetFacadesVisible(false);
}

void ABuildingInteriorVolume::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (!Player) return;

	Player->NeedsComponent->bIsIndoors = false;
	Player->bIsInsideDepthBuilding = false;
	if (OwningGenerator) OwningGenerator->SetFacadesVisible(true);
}
