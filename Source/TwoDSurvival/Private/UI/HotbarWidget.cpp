// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HotbarWidget.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

void UHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		CachedHotbarComp = Pawn->FindComponentByClass<UHotbarComponent>();
	}

	if (CachedHotbarComp)
	{
		CachedHotbarComp->OnHotbarChanged.AddDynamic(this, &UHotbarWidget::OnHotbarChanged);
	}

	if (!SlotContainer || !WidgetTree || !CachedHotbarComp) return;

	const int32 SlotCount = CachedHotbarComp->HotbarSlotCount;
	SlotBorders.Reserve(SlotCount);
	SlotIcons.Reserve(SlotCount);
	SlotKeyLabels.Reserve(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		// Border wrapping each slot — used for highlight
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			*FString::Printf(TEXT("SlotBorder_%d"), i));
		Border->SetPadding(FMargin(4.f));
		Border->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 0.8f));

		// Overlay so we can stack the key label on top of the icon
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			*FString::Printf(TEXT("SlotOverlay_%d"), i));
		Border->SetContent(Overlay);

		// Icon image for the item
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("SlotIcon_%d"), i));
		Icon->SetDesiredSizeOverride(FVector2D(48.f, 48.f));
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);

		// Key number label (1-6) in the corner
		UTextBlock* KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("SlotKey_%d"), i));
		KeyLabel->SetText(FText::AsNumber(i + 1));
		FSlateFontInfo FontInfo = KeyLabel->GetFont();
		FontInfo.Size = 10;
		KeyLabel->SetFont(FontInfo);
		KeyLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 0.8f)));
		UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(KeyLabel);
		KeySlot->SetHorizontalAlignment(HAlign_Left);
		KeySlot->SetVerticalAlignment(VAlign_Top);

		// Add to the horizontal container with some padding between slots
		UHorizontalBoxSlot* HBSlot = SlotContainer->AddChildToHorizontalBox(Border);
		HBSlot->SetPadding(FMargin(2.f));

		SlotBorders.Add(Border);
		SlotIcons.Add(Icon);
		SlotKeyLabels.Add(KeyLabel);
	}

	RefreshSlots();
}

void UHotbarWidget::RefreshSlots()
{
	if (!CachedHotbarComp) return;

	for (int32 i = 0; i < SlotBorders.Num(); ++i)
	{
		UItemDefinition* ItemDef = CachedHotbarComp->GetHotbarSlotItem(i);

		// Update icon
		if (SlotIcons.IsValidIndex(i) && SlotIcons[i])
		{
			if (ItemDef && ItemDef->Icon)
			{
				SlotIcons[i]->SetBrushFromTexture(ItemDef->Icon);
				SlotIcons[i]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				SlotIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Highlight active slot border
		if (SlotBorders.IsValidIndex(i) && SlotBorders[i])
		{
			const bool bIsActive = (i == CachedHotbarComp->ActiveSlotIndex);
			SlotBorders[i]->SetBrushColor(bIsActive
				? FLinearColor(0.8f, 0.6f, 0.1f, 0.9f)   // Gold highlight for active
				: FLinearColor(0.15f, 0.15f, 0.15f, 0.8f)); // Dark for inactive
		}
	}
}

void UHotbarWidget::OnHotbarChanged()
{
	RefreshSlots();
}
