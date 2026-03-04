// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NeedsWarningWidget.h"
#include "Components/NeedsComponent.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

void UNeedsWarningWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Find NeedsComponent on the player pawn and bind to its delegate
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		if (UNeedsComponent* NeedsComp = Pawn->FindComponentByClass<UNeedsComponent>())
		{
			WarningThreshold = NeedsComp->WarningThreshold;

			// Cache initial values
			CachedValues[0] = NeedsComp->GetNeedValue(ENeedType::Hunger);
			CachedValues[1] = NeedsComp->GetNeedValue(ENeedType::Thirst);
			CachedValues[2] = NeedsComp->GetNeedValue(ENeedType::Fatigue);

			NeedsComp->OnNeedChanged.AddDynamic(this, &UNeedsWarningWidget::OnNeedChanged);
		}
	}

	if (NeedsContainer && WidgetTree)
	{
		BuildSlots();
		RefreshAll();
	}

	// Position on the right side of the screen
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	SetPositionInViewport(FVector2D(ViewportSize.X - 80.f, 200.f), false);
}

void UNeedsWarningWidget::BuildSlots()
{
	// Per-need config: label text and fill colour
	const FText   Labels[3]  = { FText::FromString("H"), FText::FromString("T"), FText::FromString("F") };
	const FLinearColor Colors[3] = {
		FLinearColor(0.9f, 0.1f, 0.1f, 1.f),   // Hunger — red
		FLinearColor(0.1f, 0.4f, 0.9f, 1.f),   // Thirst — blue
		FLinearColor(0.9f, 0.8f, 0.1f, 1.f),   // Fatigue — yellow
	};

	NeedIcons.Reserve(3);
	NeedBars.Reserve(3);

	for (int32 i = 0; i < 3; ++i)
	{
		// Outer overlay — this is what we collapse/show
		UOverlay* Icon = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			*FString::Printf(TEXT("NeedIcon_%d"), i));
		Icon->SetVisibility(ESlateVisibility::Collapsed);

		// SizeBox constrains the icon to a fixed width/height
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			*FString::Printf(TEXT("NeedSize_%d"), i));
		SizeBox->SetWidthOverride(24.f);
		SizeBox->SetHeightOverride(64.f);
		UOverlaySlot* BoxSlot = Icon->AddChildToOverlay(SizeBox);
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
		BoxSlot->SetVerticalAlignment(VAlign_Fill);

		// Progress bar (vertical fill) inside the size box
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),
			*FString::Printf(TEXT("NeedBar_%d"), i));
		Bar->SetFillColorAndOpacity(Colors[i]);
		Bar->SetBarFillType(EProgressBarFillType::BottomToTop);
		Bar->SetPercent(1.f);
		SizeBox->SetContent(Bar);

		// Small letter label in the centre
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("NeedLabel_%d"), i));
		Label->SetText(Labels[i]);
		FSlateFontInfo FontInfo = Label->GetFont();
		FontInfo.Size = 10;
		Label->SetFont(FontInfo);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UOverlaySlot* LabelSlot = Icon->AddChildToOverlay(Label);
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);

		// Add to vertical container with small padding between icons
		UVerticalBoxSlot* VBSlot = NeedsContainer->AddChildToVerticalBox(Icon);
		VBSlot->SetPadding(FMargin(2.f));

		NeedIcons.Add(Icon);
		NeedBars.Add(Bar);
	}
}

void UNeedsWarningWidget::RefreshAll()
{
	for (int32 i = 0; i < 3; ++i)
	{
		if (!NeedIcons.IsValidIndex(i) || !NeedBars.IsValidIndex(i)) continue;

		const float Value = CachedValues[i];

		if (NeedBars[i])
		{
			NeedBars[i]->SetPercent(Value / 100.f);
		}

		if (NeedIcons[i])
		{
			const bool bShowWarning = Value < WarningThreshold;
			NeedIcons[i]->SetVisibility(
				bShowWarning ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UNeedsWarningWidget::OnNeedChanged(ENeedType NeedType, float CurrentValue, float MaxValue, bool bIsWarning)
{
	CachedValues[static_cast<int32>(NeedType)] = CurrentValue;
	RefreshAll();
}
