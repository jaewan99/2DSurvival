// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HealthHUDWidget.h"
#include "UI/BodyPartRowWidget.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UHealthHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		CachedHealthComp = Pawn->FindComponentByClass<UHealthComponent>();
	}

	if (CachedHealthComp)
	{
		CachedHealthComp->OnBodyPartDamaged.AddDynamic(this, &UHealthHUDWidget::OnHealthChanged);
	}

	RefreshAll();
}

void UHealthHUDWidget::InitDragPosition(FVector2D ViewportPos)
{
	WidgetViewportPos = ViewportPos;
}

void UHealthHUDWidget::RefreshAll()
{
	if (!CachedHealthComp) return;

	if (Row_Head)     Row_Head->SetData(FName("Head"),      CachedHealthComp->GetHealthPercent(EBodyPart::Head));
	if (Row_Body)     Row_Body->SetData(FName("Body"),      CachedHealthComp->GetHealthPercent(EBodyPart::Body));
	if (Row_LeftArm)  Row_LeftArm->SetData(FName("L.Arm"),  CachedHealthComp->GetHealthPercent(EBodyPart::LeftArm));
	if (Row_RightArm) Row_RightArm->SetData(FName("R.Arm"), CachedHealthComp->GetHealthPercent(EBodyPart::RightArm));
	if (Row_LeftLeg)  Row_LeftLeg->SetData(FName("L.Leg"),  CachedHealthComp->GetHealthPercent(EBodyPart::LeftLeg));
	if (Row_RightLeg) Row_RightLeg->SetData(FName("R.Leg"), CachedHealthComp->GetHealthPercent(EBodyPart::RightLeg));
}

void UHealthHUDWidget::OnHealthChanged(EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken)
{
	RefreshAll();
}

FReply UHealthHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TitleBar)
	{
		// Check if the click is within the TitleBar bounds
		const FGeometry TitleGeo = TitleBar->GetCachedGeometry();
		const FVector2D LocalPos = TitleGeo.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D TitleSize = TitleGeo.GetLocalSize();

		if (LocalPos.X >= 0.f && LocalPos.Y >= 0.f && LocalPos.X <= TitleSize.X && LocalPos.Y <= TitleSize.Y)
		{
			bIsDragging = true;
			// WidgetViewportPos is already correct — set by InitDragPosition on spawn
			// and kept in sync by NativeOnMouseMove on every drag frame.
			LastMousePos = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return FReply::Unhandled();
}

FReply UHealthHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging)
	{
		// Delta-based drag — add mouse movement to cached position each frame.
		// Dividing by DPI converts screen-pixel delta to viewport-space delta.
		const FVector2D MouseDelta = InMouseEvent.GetScreenSpacePosition() - LastMousePos;
		LastMousePos = InMouseEvent.GetScreenSpacePosition();

		const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
		WidgetViewportPos += MouseDelta / DPI;
		SetPositionInViewport(WidgetViewportPos, false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UHealthHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}
