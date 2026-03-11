// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SkillComponent.h"
#include "SkillRowWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * A single row in the Skill HUD showing one skill's name, level, and XP progress bar.
 * Create a Blueprint child (WBP_SkillRow) and add:
 *   SkillLabel  (TextBlock)   — skill name e.g. "Combat"
 *   LevelText   (TextBlock)   — e.g. "Lv 2" or "MAX"
 *   XPBar       (ProgressBar) — 0–1 fill toward next level
 *   XPText      (TextBlock, optional) — e.g. "75 / 100"
 *
 * Override OnLevelUp in Blueprint to play a flash / level-up animation.
 */
UCLASS()
class TWODSURVIVAL_API USkillRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* SkillLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* LevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* XPBar;

	/** Optional — shows "CurrentXP / XPForNext" or "MAX" when at max level. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* XPText;

	/**
	 * Refresh this row with the latest skill data.
	 * @param Skill       Which skill this row represents (drives bar color).
	 * @param Level       Current level (1–MaxLevel).
	 * @param CurrentXP   XP accumulated within the current level.
	 * @param XPForNext   XP needed to reach the next level (-1 when at max).
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetData(ESkillType Skill, int32 Level, int32 CurrentXP, int32 XPForNext);

	/**
	 * Called by USkillHUDWidget when this skill levels up.
	 * Override in Blueprint to play a flash or level-up animation.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnLevelUp(int32 NewLevel);
};
