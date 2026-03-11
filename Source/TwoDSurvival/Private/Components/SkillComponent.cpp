// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SkillComponent.h"
#include "Character/BaseCharacter.h"
#include "Interaction/InteractionComponent.h"

USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize all skills to Level 1, 0 XP.
	Levels.Init(1, SkillCount);
	XP.Init(0, SkillCount);

	// Cache the owner's base interaction radius so ApplyInteractionRangeBonus
	// always scales from the original value, not a previously scaled one.
	if (ABaseCharacter* Owner = Cast<ABaseCharacter>(GetOwner()))
	{
		if (Owner->InteractionComponent)
		{
			BaseInteractionRadius = Owner->InteractionComponent->DetectionRadius;
		}
	}
}

void USkillComponent::AddXP(ESkillType Skill, int32 Amount)
{
	const int32 Idx = (int32)Skill;
	if (!Levels.IsValidIndex(Idx)) return;
	if (Levels[Idx] >= MaxLevel) return;  // Already maxed — no more XP

	XP[Idx] += Amount;

	// Level up as many times as the XP allows (unlikely to multi-level in one pickup, but correct).
	while (Levels[Idx] < MaxLevel && XP[Idx] >= XPThreshold(Levels[Idx]))
	{
		XP[Idx] -= XPThreshold(Levels[Idx]);
		Levels[Idx]++;

		OnSkillLevelUp.Broadcast(Skill, Levels[Idx]);
		UE_LOG(LogTemp, Log, TEXT("SkillComponent: %s reached Level %d!"),
			*UEnum::GetValueAsString(Skill), Levels[Idx]);

		// Apply level-up side effects.
		if (Skill == ESkillType::Scavenging && Levels[Idx] == 3)
		{
			ApplyInteractionRangeBonus();
		}
	}

	const int32 XPForNext = GetXPForNextLevel(Skill);
	OnSkillXPChanged.Broadcast(Skill, XP[Idx], XPForNext);
}

int32 USkillComponent::GetLevel(ESkillType Skill) const
{
	const int32 Idx = (int32)Skill;
	return Levels.IsValidIndex(Idx) ? Levels[Idx] : 1;
}

int32 USkillComponent::GetCurrentXP(ESkillType Skill) const
{
	const int32 Idx = (int32)Skill;
	return XP.IsValidIndex(Idx) ? XP[Idx] : 0;
}

int32 USkillComponent::GetXPForNextLevel(ESkillType Skill) const
{
	const int32 Idx = (int32)Skill;
	if (!Levels.IsValidIndex(Idx)) return -1;
	if (Levels[Idx] >= MaxLevel) return -1;
	return XPThreshold(Levels[Idx]);
}

float USkillComponent::GetCombatDamageMultiplier() const
{
	// Lv2+: 10% damage bonus.
	return GetLevel(ESkillType::Combat) >= 2 ? 1.1f : 1.f;
}

float USkillComponent::GetAttackPlayRateMultiplier() const
{
	// Lv3: animations play 1/0.85 ≈ 1.18× faster, reducing effective cooldown by ~15%.
	return GetLevel(ESkillType::Combat) >= 3 ? (1.f / 0.85f) : 1.f;
}

bool USkillComponent::RollDoubleOutput() const
{
	// Crafting Lv3: 10% chance.
	if (GetLevel(ESkillType::Crafting) < 3) return false;
	return FMath::FRand() < 0.10f;
}

int32 USkillComponent::GetExtraLootRolls() const
{
	// Scavenging Lv2+: one extra roll per enemy.
	return GetLevel(ESkillType::Scavenging) >= 2 ? 1 : 0;
}

void USkillComponent::GetSaveData(TArray<int32>& OutLevels, TArray<int32>& OutXP) const
{
	OutLevels = Levels;
	OutXP     = XP;
}

void USkillComponent::ApplySaveData(const TArray<int32>& InLevels, const TArray<int32>& InXP)
{
	if (InLevels.Num() != SkillCount || InXP.Num() != SkillCount) return;

	Levels = InLevels;
	XP     = InXP;

	// Re-apply any permanent side effects for loaded levels.
	if (GetLevel(ESkillType::Scavenging) >= 3)
	{
		ApplyInteractionRangeBonus();
	}
}

void USkillComponent::ApplyInteractionRangeBonus()
{
	ABaseCharacter* Owner = Cast<ABaseCharacter>(GetOwner());
	if (!Owner || !Owner->InteractionComponent) return;

	Owner->InteractionComponent->SetDetectionRadius(BaseInteractionRadius * 1.5f);

	UE_LOG(LogTemp, Log, TEXT("SkillComponent: Scavenging Lv3 — interaction radius set to %.0f"),
		BaseInteractionRadius * 1.5f);
}
