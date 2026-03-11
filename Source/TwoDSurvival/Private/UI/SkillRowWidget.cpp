// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SkillRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void USkillRowWidget::SetData(ESkillType Skill, int32 Level, int32 CurrentXP, int32 XPForNext)
{
	// ── Label ────────────────────────────────────────────────────────────
	if (SkillLabel)
	{
		FString SkillName;
		switch (Skill)
		{
		case ESkillType::Combat:     SkillName = TEXT("Combat");     break;
		case ESkillType::Crafting:   SkillName = TEXT("Crafting");   break;
		case ESkillType::Scavenging: SkillName = TEXT("Scavenging"); break;
		default:                     SkillName = TEXT("Unknown");    break;
		}
		SkillLabel->SetText(FText::FromString(SkillName));
	}

	// ── Level text ───────────────────────────────────────────────────────
	const bool bIsMaxLevel = (XPForNext < 0);
	if (LevelText)
	{
		const FString LevelStr = bIsMaxLevel
			? FString::Printf(TEXT("Lv %d  MAX"), Level)
			: FString::Printf(TEXT("Lv %d"), Level);
		LevelText->SetText(FText::FromString(LevelStr));
	}

	// ── XP bar ───────────────────────────────────────────────────────────
	if (XPBar)
	{
		const float Percent = bIsMaxLevel
			? 1.f
			: (XPForNext > 0 ? FMath::Clamp((float)CurrentXP / (float)XPForNext, 0.f, 1.f) : 0.f);

		XPBar->SetPercent(Percent);

		// Unique color per skill so the three bars are visually distinct.
		FLinearColor BarColor;
		switch (Skill)
		{
		case ESkillType::Combat:     BarColor = FLinearColor(0.85f, 0.15f, 0.10f); break; // Red
		case ESkillType::Crafting:   BarColor = FLinearColor(0.10f, 0.75f, 0.45f); break; // Teal
		case ESkillType::Scavenging: BarColor = FLinearColor(0.90f, 0.70f, 0.10f); break; // Amber
		default:                     BarColor = FLinearColor::White;                break;
		}

		// Dim the bar when maxed out.
		if (bIsMaxLevel)
		{
			BarColor *= 0.6f;
			BarColor.A = 1.f;
		}

		XPBar->SetFillColorAndOpacity(BarColor);
	}

	// ── Optional XP text ─────────────────────────────────────────────────
	if (XPText)
	{
		const FString XPStr = bIsMaxLevel
			? TEXT("MAX")
			: FString::Printf(TEXT("%d / %d"), CurrentXP, XPForNext);
		XPText->SetText(FText::FromString(XPStr));
	}
}
