// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/DisassembleComponent.h"
#include "Character/BaseCharacter.h"
#include "World/WorldItem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UDisassembleComponent::UDisassembleComponent()
{
	Priority = 5;
}

FText UDisassembleComponent::GetPrompt() const
{
	FString Name = ActionLabel.IsEmpty() ? TEXT("Object") : ActionLabel.ToString();
	return FText::FromString(FString::Printf(TEXT("Disassemble [%s]"), *Name));
}

void UDisassembleComponent::Execute(ABaseCharacter* Interactor)
{
	if (SFX_Disassemble)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_Disassemble, GetOwner()->GetActorLocation());

	SpawnYield();
	GetOwner()->Destroy();
}

EInteractionType UDisassembleComponent::GetInteractionType() const
{
	return HoldDuration > 0.f ? EInteractionType::Hold : EInteractionType::Instant;
}

float UDisassembleComponent::GetInteractionDuration() const
{
	return HoldDuration;
}

void UDisassembleComponent::SpawnYield()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (const FLootEntry& Entry : Yield)
	{
		if (!Entry.ItemDef || FMath::FRand() > Entry.DropChance) continue;

		int32 Count = FMath::RandRange(Entry.MinCount, Entry.MaxCount);
		if (Count <= 0) continue;

		FVector DropLoc = GetOwner()->GetActorLocation()
			+ FVector(FMath::RandRange(-30.f, 30.f), 0.f, 10.f);

		AWorldItem* Item = World->SpawnActor<AWorldItem>(
			AWorldItem::StaticClass(),
			FTransform(FRotator::ZeroRotator, DropLoc));

		if (Item)
		{
			Item->ItemDef = Entry.ItemDef;
			Item->Quantity = Count;
		}
	}
}
