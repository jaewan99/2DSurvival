// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SkillHUDWidget.h"
#include "UI/SkillRowWidget.h"
#include "Components/SkillComponent.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Pawn.h"

void USkillHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		CachedSkillComp = Pawn->FindComponentByClass<USkillComponent>();
	}

	if (CachedSkillComp)
	{
		CachedSkillComp->OnSkillXPChanged.AddDynamic(this, &USkillHUDWidget::OnSkillXPChanged);
		CachedSkillComp->OnSkillLevelUp.AddDynamic(this, &USkillHUDWidget::OnSkillLevelUp);
	}

	RefreshAll();
}

void USkillHUDWidget::InitDragPosition(FVector2D ViewportPos)
{
	WidgetViewportPos = ViewportPos;
}

void USkillHUDWidget::RefreshAll()
{
	if (!CachedSkillComp) return;

	const ESkillType Skills[] = {
		ESkillType::Combat,
		ESkillType::Crafting,
		ESkillType::Scavenging
	};

	for (ESkillType Skill : Skills)
	{
		if (USkillRowWidget* Row = GetRowForSkill(Skill))
		{
			Row->SetData(
				Skill,
				CachedSkillComp->GetLevel(Skill),
				CachedSkillComp->GetCurrentXP(Skill),
				CachedSkillComp->GetXPForNextLevel(Skill));
		}
	}
}

void USkillHUDWidget::OnSkillXPChanged(ESkillType Skill, int32 CurrentXP, int32 XPForNext)
{
	// Only refresh the one row that changed — avoids redundant updates.
	if (USkillRowWidget* Row = GetRowForSkill(Skill))
	{
		Row->SetData(
			Skill,
			CachedSkillComp ? CachedSkillComp->GetLevel(Skill) : 1,
			CurrentXP,
			XPForNext);
	}
}

void USkillHUDWidget::OnSkillLevelUp(ESkillType Skill, int32 NewLevel)
{
	// Refresh the row then fire the Blueprint event so an animation can play.
	OnSkillXPChanged(
		Skill,
		CachedSkillComp ? CachedSkillComp->GetCurrentXP(Skill) : 0,
		CachedSkillComp ? CachedSkillComp->GetXPForNextLevel(Skill) : -1);

	if (USkillRowWidget* Row = GetRowForSkill(Skill))
	{
		Row->OnLevelUp(NewLevel);
	}
}

USkillRowWidget* USkillHUDWidget::GetRowForSkill(ESkillType Skill) const
{
	switch (Skill)
	{
	case ESkillType::Combat:     return Row_Combat;
	case ESkillType::Crafting:   return Row_Crafting;
	case ESkillType::Scavenging: return Row_Scavenging;
	default:                     return nullptr;
	}
}

// ── Drag-to-move (mirrors HealthHUDWidget exactly) ────────────────────────────

FReply USkillHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TitleBar)
	{
		const FGeometry TitleGeo = TitleBar->GetCachedGeometry();
		const FVector2D LocalPos = TitleGeo.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D TitleSize = TitleGeo.GetLocalSize();

		if (LocalPos.X >= 0.f && LocalPos.Y >= 0.f &&
			LocalPos.X <= TitleSize.X && LocalPos.Y <= TitleSize.Y)
		{
			bIsDragging  = true;
			LastMousePos = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return FReply::Unhandled();
}

FReply USkillHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D MouseDelta = InMouseEvent.GetScreenSpacePosition() - LastMousePos;
		LastMousePos = InMouseEvent.GetScreenSpacePosition();

		const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
		WidgetViewportPos += MouseDelta / DPI;
		SetPositionInViewport(WidgetViewportPos, false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply USkillHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}
