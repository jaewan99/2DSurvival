// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/NeedsComponent.h"
#include "Character/BaseCharacter.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"

UNeedsComponent::UNeedsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNeedsComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChar = Cast<ABaseCharacter>(GetOwner());
}

void UNeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsSleeping)
	{
		// While sleeping: restore fatigue at 3x the normal rate
		const float OldFatigue = Fatigue;
		Fatigue = FMath::Clamp(Fatigue + FatigueDrainRate * 3.f * SleepTimeScaleMultiplier * DeltaTime, 0.f, 100.f);

		if (!FMath::IsNearlyEqual(Fatigue, OldFatigue, 0.001f))
		{
			OnNeedChanged.Broadcast(ENeedType::Fatigue, Fatigue, 100.f, Fatigue < WarningThreshold);
		}

		// Auto-wake when Fatigue hits 100, but only if the player actually needed sleep
		// (SleepStartFatigue < 95). Prevents immediate wake when sleeping with near-full fatigue.
		if (Fatigue >= 100.f && OldFatigue < 100.f && SleepStartFatigue < 95.f && OwnerChar)
		{
			OwnerChar->StopSleeping();
		}
		return;
	}

	DrainNeed(ENeedType::Hunger,  DeltaTime);
	DrainNeed(ENeedType::Thirst,  DeltaTime);
	DrainNeed(ENeedType::Fatigue, DeltaTime);
	ApplyCriticalEffects(DeltaTime);

	// Passive mood drain + extra per critical need
	float MoodDrain = MoodPassiveDrainRate;
	if (Hunger  <= 0.f) MoodDrain += CriticalMoodDrainRate;
	if (Thirst  <= 0.f) MoodDrain += CriticalMoodDrainRate;
	if (Fatigue <= 0.f) MoodDrain += CriticalMoodDrainRate;
	ModifyMood(-MoodDrain * DeltaTime);

	// Weather modifiers — only apply when outdoors.
	if (!bIsIndoors)
	{
		if (WeatherThirstBoostRate > 0.f)
		{
			const float OldThirst = Thirst;
			Thirst = FMath::Clamp(Thirst + WeatherThirstBoostRate * DeltaTime, 0.f, 100.f);
			if (!FMath::IsNearlyEqual(Thirst, OldThirst, 0.001f))
			{
				OnNeedChanged.Broadcast(ENeedType::Thirst, Thirst, 100.f, Thirst < WarningThreshold);
			}
		}
		if (WeatherMoodDrainRate > 0.f)
		{
			ModifyMood(-WeatherMoodDrainRate * DeltaTime);
		}
	}
}

void UNeedsComponent::DrainNeed(ENeedType NeedType, float DeltaTime)
{
	float* ValuePtr = nullptr;
	float  Rate     = 0.f;

	switch (NeedType)
	{
	case ENeedType::Hunger:  ValuePtr = &Hunger;  Rate = HungerDrainRate;  break;
	case ENeedType::Thirst:  ValuePtr = &Thirst;  Rate = ThirstDrainRate;  break;
	case ENeedType::Fatigue: ValuePtr = &Fatigue; Rate = FatigueDrainRate; break;
	default: return;
	}

	const float OldValue = *ValuePtr;
	*ValuePtr = FMath::Clamp(*ValuePtr - Rate * (bIsActiveMovement ? ActiveMultiplier : 1.f) * DeltaTime, 0.f, 100.f);

	if (!FMath::IsNearlyEqual(*ValuePtr, OldValue, 0.001f))
	{
		OnNeedChanged.Broadcast(NeedType, *ValuePtr, 100.f, *ValuePtr < WarningThreshold);
	}
}

void UNeedsComponent::ApplyCriticalEffects(float DeltaTime)
{
	if (!OwnerChar || !OwnerChar->HealthComponent) return;

	if (Hunger  <= 0.f) OwnerChar->HealthComponent->ApplyDamage(EBodyPart::Body, CriticalHPDrain * DeltaTime);
	if (Thirst  <= 0.f) OwnerChar->HealthComponent->ApplyDamage(EBodyPart::Body, CriticalHPDrain * DeltaTime);
	if (Fatigue <= 0.f) OwnerChar->HealthComponent->ApplyDamage(EBodyPart::Body, CriticalHPDrain * DeltaTime);
}

void UNeedsComponent::RestoreNeed(ENeedType NeedType, float Amount)
{
	float* ValuePtr = nullptr;
	switch (NeedType)
	{
	case ENeedType::Hunger:  ValuePtr = &Hunger;  break;
	case ENeedType::Thirst:  ValuePtr = &Thirst;  break;
	case ENeedType::Fatigue: ValuePtr = &Fatigue; break;
	default: return;
	}

	const float OldValue = *ValuePtr;
	*ValuePtr = FMath::Clamp(*ValuePtr + Amount, 0.f, 100.f);

	if (!FMath::IsNearlyEqual(*ValuePtr, OldValue, 0.001f))
	{
		OnNeedChanged.Broadcast(NeedType, *ValuePtr, 100.f, *ValuePtr < WarningThreshold);
	}
}

void UNeedsComponent::SetNeedValue(ENeedType NeedType, float Value)
{
	float* ValuePtr = nullptr;
	switch (NeedType)
	{
	case ENeedType::Hunger:  ValuePtr = &Hunger;  break;
	case ENeedType::Thirst:  ValuePtr = &Thirst;  break;
	case ENeedType::Fatigue: ValuePtr = &Fatigue; break;
	default: return;
	}

	*ValuePtr = FMath::Clamp(Value, 0.f, 100.f);
	OnNeedChanged.Broadcast(NeedType, *ValuePtr, 100.f, *ValuePtr < WarningThreshold);
}

float UNeedsComponent::GetNeedValue(ENeedType NeedType) const
{
	switch (NeedType)
	{
	case ENeedType::Hunger:  return Hunger;
	case ENeedType::Thirst:  return Thirst;
	case ENeedType::Fatigue: return Fatigue;
	default: return 0.f;
	}
}

float UNeedsComponent::GetSpeedMultiplier() const
{
	return (Hunger < WarningThreshold || Thirst < WarningThreshold || Fatigue < WarningThreshold)
		? 0.7f : 1.0f;
}

float UNeedsComponent::GetDamageMultiplier() const
{
	return (Hunger < WarningThreshold || Thirst < WarningThreshold || Fatigue < WarningThreshold)
		? 0.8f : 1.0f;
}

bool UNeedsComponent::IsHealthRegenBlocked() const
{
	return (Hunger < WarningThreshold || Thirst < WarningThreshold || Fatigue < WarningThreshold);
}

void UNeedsComponent::SetActiveMovement(bool bActive)
{
	bIsActiveMovement = bActive;
}

void UNeedsComponent::SetWeatherModifiers(float ThirstBoost, float MoodDrain)
{
	WeatherThirstBoostRate = FMath::Max(0.f, ThirstBoost);
	WeatherMoodDrainRate   = FMath::Max(0.f, MoodDrain);
}

void UNeedsComponent::ModifyMood(float Delta)
{
	const float OldMood = Mood;
	Mood = FMath::Clamp(Mood + Delta, 0.f, 100.f);
	if (!FMath::IsNearlyEqual(Mood, OldMood, 0.001f))
	{
		OnMoodChanged.Broadcast(Mood);
	}
}

float UNeedsComponent::GetMood() const
{
	return Mood;
}

void UNeedsComponent::SetMood(float Value)
{
	Mood = FMath::Clamp(Value, 0.f, 100.f);
	OnMoodChanged.Broadcast(Mood);
}
