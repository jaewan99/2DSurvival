// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/StatusEffectComponent.h"
#include "Components/NeedsComponent.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"
#include "World/WeatherManager.h"
#include "Kismet/GameplayStatics.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedNeeds  = GetOwner()->FindComponentByClass<UNeedsComponent>();
	CachedHealth = GetOwner()->FindComponentByClass<UHealthComponent>();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWeatherManager::StaticClass(), Found);
	if (Found.Num() > 0) CachedWeather = Cast<AWeatherManager>(Found[0]);
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void UStatusEffectComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveEffects.IsEmpty() && !CachedWeather) return;

	const bool bIndoors  = CachedNeeds  && CachedNeeds->bIsIndoors;
	const bool bRaining  = CachedWeather && CachedWeather->bIsRaining;
	const bool bSnowing  = CachedWeather && CachedWeather->bIsSnowing;

	// ── Auto-apply / manage Wet ───────────────────────────────────────────
	const bool bShouldBeWet = (bRaining || bSnowing) && !bIndoors;
	if (FActiveStatusEffect* WetFx = GetEffectPtr(EStatusEffect::Wet))
	{
		if (bShouldBeWet)
		{
			WetFx->RemainingDuration = -1.f;  // Keep refreshed while outdoors in rain/snow
		}
		else if (WetFx->RemainingDuration < 0.f)
		{
			WetFx->RemainingDuration = WetDryTime;  // Begin drying countdown
		}
	}
	else if (bShouldBeWet)
	{
		ApplyEffect(EStatusEffect::Wet, 1.f, -1.f);
	}

	// ── Cold exposure: Wet + Snow outdoors → Frostbite ───────────────────
	const bool bColdConditions = bSnowing && !bIndoors;
	if (bColdConditions && (HasEffect(EStatusEffect::Wet) || bSnowing))
	{
		ColdExposureAccum += DeltaTime;
		if (ColdExposureAccum >= FrostbiteThreshold && !HasEffect(EStatusEffect::Frostbite))
		{
			ApplyEffect(EStatusEffect::Frostbite, 1.f, -1.f);
		}
	}
	else
	{
		ColdExposureAccum = FMath::Max(0.f, ColdExposureAccum - DeltaTime * 0.5f);  // Slowly recover
	}

	// ── Frostbite in cold → Hypothermia ──────────────────────────────────
	if (HasEffect(EStatusEffect::Frostbite) && bColdConditions)
	{
		HypothermiaAccum += DeltaTime;
		if (HypothermiaAccum >= HypothermiaThreshold && !HasEffect(EStatusEffect::Hypothermia))
		{
			ApplyEffect(EStatusEffect::Hypothermia, 1.f, -1.f);
		}
	}
	else
	{
		HypothermiaAccum = FMath::Max(0.f, HypothermiaAccum - DeltaTime * 0.25f);
	}

	// ── Bleeding → Infected progression ──────────────────────────────────
	if (HasEffect(EStatusEffect::Bleeding))
	{
		BleedingTimer += DeltaTime;
		if (BleedingTimer >= BleedToInfectTime && !HasEffect(EStatusEffect::Infected))
		{
			ApplyEffect(EStatusEffect::Infected, 1.f, -1.f);
		}
	}
	else
	{
		BleedingTimer = 0.f;
	}

	// ── Per-effect consequences and duration tick ─────────────────────────
	bool bChanged = false;
	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		FActiveStatusEffect& Fx = ActiveEffects[i];

		switch (Fx.Type)
		{
		case EStatusEffect::Bleeding:
			if (CachedHealth)
				CachedHealth->ApplyDamage(EBodyPart::Body, BleedDrainRate * DeltaTime);
			break;

		case EStatusEffect::Infected:
			if (CachedHealth)
				CachedHealth->ApplyDamage(EBodyPart::Body, InfectDrainRate * DeltaTime);
			break;

		case EStatusEffect::Poisoned:
			if (CachedHealth)
				CachedHealth->ApplyDamage(EBodyPart::Body, PoisonDrainRate * DeltaTime);
			if (CachedNeeds)
			{
				CachedNeeds->ApplyDrain(ENeedType::Hunger, 0.055f * DeltaTime);  // 2× base rate
				CachedNeeds->ApplyDrain(ENeedType::Thirst, 0.075f * DeltaTime);
			}
			break;

		case EStatusEffect::Hypothermia:
			if (CachedHealth)
				CachedHealth->ApplyDamage(EBodyPart::Body, HypothermiaDrainRate * DeltaTime);
			if (CachedNeeds)
				CachedNeeds->ApplyDrain(ENeedType::Fatigue, 0.04f * DeltaTime);  // 2× base fatigue
			break;

		case EStatusEffect::Wet:
			if (CachedNeeds)
				CachedNeeds->ApplyDrain(ENeedType::Thirst, 0.0375f * DeltaTime);  // 1.5× base thirst
			break;

		default:
			break;
		}

		// Tick duration-based effects.
		if (Fx.RemainingDuration > 0.f)
		{
			Fx.RemainingDuration -= DeltaTime;
			if (Fx.RemainingDuration <= 0.f)
			{
				ActiveEffects.RemoveAt(i);
				bChanged = true;
			}
		}
	}

	if (bChanged) OnStatusEffectsChanged.Broadcast();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UStatusEffectComponent::ApplyEffect(EStatusEffect Effect, float Severity, float Duration)
{
	if (Effect == EStatusEffect::None) return;

	// Replace if already active with a higher severity; otherwise leave unchanged.
	if (FActiveStatusEffect* Existing = GetEffectPtr(Effect))
	{
		if (Severity > Existing->Severity)
		{
			Existing->Severity = Severity;
			Existing->RemainingDuration = Duration;
			OnStatusEffectsChanged.Broadcast();
		}
		return;
	}

	FActiveStatusEffect New;
	New.Type              = Effect;
	New.Severity          = Severity;
	New.RemainingDuration = Duration;
	ActiveEffects.Add(New);
	OnStatusEffectsChanged.Broadcast();
}

void UStatusEffectComponent::RemoveEffect(EStatusEffect Effect)
{
	const int32 Removed = ActiveEffects.RemoveAll(
		[Effect](const FActiveStatusEffect& E){ return E.Type == Effect; });
	if (Removed > 0)
	{
		// Reset associated accumulators so re-exposure starts clean.
		if (Effect == EStatusEffect::Bleeding)  BleedingTimer     = 0.f;
		if (Effect == EStatusEffect::Frostbite) HypothermiaAccum  = 0.f;
		if (Effect == EStatusEffect::Wet)       ColdExposureAccum = 0.f;

		OnStatusEffectsChanged.Broadcast();
	}
}

bool UStatusEffectComponent::HasEffect(EStatusEffect Effect) const
{
	for (const FActiveStatusEffect& E : ActiveEffects)
		if (E.Type == Effect) return true;
	return false;
}

float UStatusEffectComponent::GetSpeedMultiplier() const
{
	float Mult = 1.f;
	if (HasEffect(EStatusEffect::BrokenBone))  Mult *= 0.5f;
	if (HasEffect(EStatusEffect::Frostbite))   Mult *= 0.85f;
	if (HasEffect(EStatusEffect::Hypothermia)) Mult *= 0.7f;
	if (HasEffect(EStatusEffect::Concussion))  Mult *= 0.8f;
	return Mult;
}

float UStatusEffectComponent::GetDamageMultiplier() const
{
	float Mult = 1.f;
	if (HasEffect(EStatusEffect::Frostbite))  Mult *= 0.8f;
	if (HasEffect(EStatusEffect::Poisoned))   Mult *= 0.9f;
	if (HasEffect(EStatusEffect::Concussion)) Mult *= 0.7f;
	return Mult;
}

FActiveStatusEffect* UStatusEffectComponent::GetEffectPtr(EStatusEffect Effect)
{
	for (FActiveStatusEffect& E : ActiveEffects)
		if (E.Type == Effect) return &E;
	return nullptr;
}
