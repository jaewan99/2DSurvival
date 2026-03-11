// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

/** The three independent skill tracks. */
UENUM(BlueprintType)
enum class ESkillType : uint8
{
	Combat     UMETA(DisplayName = "Combat"),
	Crafting   UMETA(DisplayName = "Crafting"),
	Scavenging UMETA(DisplayName = "Scavenging"),
};

/**
 * Broadcast when a skill reaches a new level.
 * Bind in UI to show a "Level Up!" notification.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillLevelUp, ESkillType, Skill, int32, NewLevel);

/**
 * Broadcast after every XP gain (even with no level-up).
 * Bind in UI to update XP bars.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillXPChanged, ESkillType, Skill, int32, CurrentXP, int32, XPForNext);

/**
 * Tracks three independent skill tracks: Combat, Crafting, and Scavenging.
 * Each starts at Level 1. XP is accumulated toward the next level threshold (Level × 100).
 *
 * Level bonuses applied by other systems (queried via GetLevel):
 *   Combat Lv2  — +10% melee damage (WeaponBase::OnHitboxOverlap)
 *   Combat Lv3  — attack animations play 15% faster (BaseCharacter::OnAttackPressed)
 *   Crafting Lv2 — unlocks tier-2 recipes (MinCraftingLevel = 2 on UCraftingRecipe)
 *   Crafting Lv3 — 10% chance to craft double output (UCraftingComponent::TryCraft)
 *   Scavenging Lv2 — +1 extra loot roll per enemy death (AEnemyBase::SpawnLoot)
 *   Scavenging Lv3 — +50% interaction detection radius (UInteractionComponent)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillComponent();

	// ── Config ────────────────────────────────────────────────────────────

	/** Maximum level any skill can reach. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills")
	int32 MaxLevel = 3;

	/** XP granted per enemy kill. Tune in BP_BaseCharacter class defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills|XP Amounts")
	int32 CombatXPPerKill = 15;

	/** XP granted per successful craft. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills|XP Amounts")
	int32 CraftingXPPerCraft = 10;

	/** XP granted per world item picked up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skills|XP Amounts")
	int32 ScavengingXPPerPickup = 5;

	// ── Delegates ─────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Skills")
	FOnSkillLevelUp OnSkillLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Skills")
	FOnSkillXPChanged OnSkillXPChanged;

	// ── Public API ────────────────────────────────────────────────────────

	/**
	 * Add XP to a skill. Automatically levels up if the threshold is crossed.
	 * Broadcasts OnSkillXPChanged (and OnSkillLevelUp on level-up).
	 * No-op if the skill is already at MaxLevel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void AddXP(ESkillType Skill, int32 Amount);

	/** Current level of the skill (starts at 1). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	int32 GetLevel(ESkillType Skill) const;

	/** XP accumulated within the current level, toward the next threshold. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	int32 GetCurrentXP(ESkillType Skill) const;

	/**
	 * Total XP needed to advance from the current level to the next.
	 * Formula: CurrentLevel × 100 (e.g. Lv1→2 = 100, Lv2→3 = 200).
	 * Returns -1 when already at MaxLevel.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	int32 GetXPForNextLevel(ESkillType Skill) const;

	// ── Bonus queries (called by other C++ systems) ────────────────────────

	/** Combat Lv1: 1.0 × | Lv2+: 1.1 × weapon damage. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	float GetCombatDamageMultiplier() const;

	/**
	 * Attack animation play-rate multiplier.
	 * Combat Lv3+: animations play at 1/0.85 ≈ 1.18× speed.
	 * Returns 1.0 at Lv1–2.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	float GetAttackPlayRateMultiplier() const;

	/**
	 * Crafting Lv3: rolls a 10% chance to craft double output.
	 * Call once per TryCraft success. Returns true if the bonus triggers.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool RollDoubleOutput() const;

	/**
	 * Extra loot roll count added to enemy loot tables on death.
	 * Scavenging Lv2+: returns 1. Lv1: returns 0.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skills")
	int32 GetExtraLootRolls() const;

	// ── Save / Load ────────────────────────────────────────────────────────

	/** Writes current Levels and per-level XP into flat arrays for serialization. */
	void GetSaveData(TArray<int32>& OutLevels, TArray<int32>& OutXP) const;

	/**
	 * Restores skill state from serialized arrays.
	 * Reapplies Scavenging Lv3 interaction range bonus if applicable.
	 */
	void ApplySaveData(const TArray<int32>& InLevels, const TArray<int32>& InXP);

protected:
	virtual void BeginPlay() override;

private:
	// Indexed by (int32)ESkillType. Size = SkillCount.
	TArray<int32> Levels;
	TArray<int32> XP;

	static constexpr int32 SkillCount = 3;

	/**
	 * Base interaction radius captured at BeginPlay.
	 * Used by ApplyInteractionRangeBonus to always scale from the original value.
	 */
	float BaseInteractionRadius = 150.f;

	/** XP required to advance from CurrentLevel to CurrentLevel+1. */
	int32 XPThreshold(int32 CurrentLevel) const { return CurrentLevel * 100; }

	/** Resizes the owning character's interaction detection sphere by ×1.5. */
	void ApplyInteractionRangeBonus();
};
