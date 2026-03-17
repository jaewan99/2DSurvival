// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventoryWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TitleText)
		TitleText->SetText(FText::FromString(TEXT("Inventory")));
}

void UInventoryWidget::InitDragPosition(FVector2D ViewportPos)
{
	WidgetViewportPos = ViewportPos;
}

void UInventoryWidget::SetTitle(const FText& NewTitle)
{
	if (TitleText)
		TitleText->SetText(NewTitle);
}

FReply UInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TitleBar)
	{
		const FGeometry TitleGeo = TitleBar->GetCachedGeometry();
		const FVector2D LocalPos = TitleGeo.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D TitleSize = TitleGeo.GetLocalSize();

		if (LocalPos.X >= 0.f && LocalPos.Y >= 0.f && LocalPos.X <= TitleSize.X && LocalPos.Y <= TitleSize.Y)
		{
			bIsDragging = true;
			LastMousePos = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return FReply::Unhandled();
}

FReply UInventoryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply UInventoryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}
