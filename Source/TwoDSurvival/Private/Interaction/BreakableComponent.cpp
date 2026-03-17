// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/BreakableComponent.h"
#include "Character/BaseCharacter.h"
#include "World/WorldItem.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

UBreakableComponent::UBreakableComponent()
{
	Priority = 10;
}

void UBreakableComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void UBreakableComponent::ApplyDamage(float Amount, ABaseCharacter* DamageSource)
{
	if (bIsBroken || Amount <= 0.f) return;

	// Wrong tool — silently ignore
	if (RequiredTool != EToolType::None && GetEquippedToolType(DamageSource) != RequiredTool) return;

	if (SFX_Hit)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_Hit, GetOwner()->GetActorLocation());

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	if (CurrentHealth <= 0.f)
		OnHealthDepleted();
}

FText UBreakableComponent::GetPrompt() const
{
	FString Name = ActionLabel.IsEmpty() ? TEXT("Object") : ActionLabel.ToString();

	FString ToolHint;
	switch (RequiredTool)
	{
		case EToolType::Hammer:  ToolHint = TEXT(" (Hammer)");  break;
		case EToolType::Axe:     ToolHint = TEXT(" (Axe)");     break;
		case EToolType::Crowbar: ToolHint = TEXT(" (Crowbar)"); break;
		default: break;
	}

	return FText::FromString(FString::Printf(TEXT("Break [%s]%s"), *Name, *ToolHint));
}

bool UBreakableComponent::IsAvailable() const
{
	return !bIsBroken;
}

bool UBreakableComponent::CanInteract(ABaseCharacter* Interactor) const
{
	// Always show prompt — tool check happens in ApplyDamage, not here.
	return true;
}

void UBreakableComponent::Execute(ABaseCharacter* Interactor)
{
	// E-key on a breakable: tell the player what they need
	if (GEngine)
	{
		FString Msg = (RequiredTool == EToolType::None)
			? TEXT("Attack this with a weapon to break it")
			: FString::Printf(TEXT("Requires a %s to break"), *UEnum::GetValueAsString(RequiredTool));
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);
	}
}

void UBreakableComponent::OnHealthDepleted()
{
	bIsBroken = true;

	if (SFX_Destroyed)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_Destroyed, GetOwner()->GetActorLocation());

	SpawnLoot();
	OnBroken.Broadcast();

	if (bDestroyOnDeath)
	{
		GetOwner()->Destroy();
		return;
	}

	// Swap to broken mesh if provided
	if (BrokenMesh)
	{
		if (UStaticMeshComponent* SM = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
			SM->SetStaticMesh(BrokenMesh);
	}
}

void UBreakableComponent::SpawnLoot()
{
	UWorld* World = GetWorld();
	if (!World || LootTable.Num() == 0) return;

	for (const FLootEntry& Entry : LootTable)
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
