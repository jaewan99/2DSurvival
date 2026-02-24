// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize each body part from the configurable max health properties.
	auto MakePart = [](float Max) -> FBodyPartHealth
	{
		FBodyPartHealth Part;
		Part.MaxHealth = Max;
		Part.CurrentHealth = Max;
		return Part;
	};

	BodyParts.Add(EBodyPart::Head,     MakePart(HeadMaxHealth));
	BodyParts.Add(EBodyPart::Body,     MakePart(BodyMaxHealth));
	BodyParts.Add(EBodyPart::LeftArm,  MakePart(ArmMaxHealth));
	BodyParts.Add(EBodyPart::RightArm, MakePart(ArmMaxHealth));
	BodyParts.Add(EBodyPart::LeftLeg,  MakePart(LegMaxHealth));
	BodyParts.Add(EBodyPart::RightLeg, MakePart(LegMaxHealth));
}

void UHealthComponent::ApplyDamage(EBodyPart Part, float Amount)
{
	if (Amount <= 0.f) return;

	FBodyPartHealth* Data = BodyParts.Find(Part);
	if (!Data) return;

	const bool bWasBroken = Data->IsBroken();
	Data->CurrentHealth = FMath::Clamp(Data->CurrentHealth - Amount, 0.f, Data->MaxHealth);
	const bool bJustBroken = !bWasBroken && Data->IsBroken();

	OnBodyPartDamaged.Broadcast(Part, Data->CurrentHealth, Data->MaxHealth, bJustBroken);
}

void UHealthComponent::RestoreHealth(EBodyPart Part, float Amount)
{
	if (Amount <= 0.f) return;

	FBodyPartHealth* Data = BodyParts.Find(Part);
	if (!Data) return;

	Data->CurrentHealth = FMath::Clamp(Data->CurrentHealth + Amount, 0.f, Data->MaxHealth);
}

float UHealthComponent::GetHealthPercent(EBodyPart Part) const
{
	const FBodyPartHealth* Data = BodyParts.Find(Part);
	if (!Data || Data->MaxHealth <= 0.f) return 0.f;
	return Data->CurrentHealth / Data->MaxHealth;
}

FBodyPartHealth UHealthComponent::GetBodyPart(EBodyPart Part) const
{
	const FBodyPartHealth* Data = BodyParts.Find(Part);
	if (Data) return *Data;
	return FBodyPartHealth();
}

bool UHealthComponent::IsDead() const
{
	const FBodyPartHealth* Head = BodyParts.Find(EBodyPart::Head);
	const FBodyPartHealth* Body = BodyParts.Find(EBodyPart::Body);
	return (Head && Head->IsBroken()) || (Body && Body->IsBroken());
}

float UHealthComponent::GetDamageMultiplier() const
{
	const float Left  = GetHealthPercent(EBodyPart::LeftArm);
	const float Right = GetHealthPercent(EBodyPart::RightArm);
	return (Left + Right) * 0.5f;
}

void UHealthComponent::SetBodyPartHealth(EBodyPart Part, float NewCurrentHealth)
{
	FBodyPartHealth* Data = BodyParts.Find(Part);
	if (!Data) return;

	Data->CurrentHealth = FMath::Clamp(NewCurrentHealth, 0.f, Data->MaxHealth);
}

float UHealthComponent::GetMovementSpeedMultiplier() const
{
	const FBodyPartHealth* Left  = BodyParts.Find(EBodyPart::LeftLeg);
	const FBodyPartHealth* Right = BodyParts.Find(EBodyPart::RightLeg);

	const bool bLeftBroken  = Left  && Left->IsBroken();
	const bool bRightBroken = Right && Right->IsBroken();

	if (bLeftBroken && bRightBroken) return 0.25f;
	if (bLeftBroken || bRightBroken) return 0.75f;
	return 1.0f;
}
