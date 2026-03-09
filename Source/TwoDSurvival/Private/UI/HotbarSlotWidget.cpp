// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HotbarSlotWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UHotbarSlotWidget::SetSlotData(UItemDefinition* ItemDef, bool bIsActive, int32 Index)
{
    CurrentItemDef = ItemDef;
    bIsActiveSlot  = bIsActive;
    SlotIndex      = Index;

    // Icon
    if (SlotIcon)
    {
        if (ItemDef && ItemDef->Icon)
        {
            SlotIcon->SetBrushFromTexture(ItemDef->Icon);
            SlotIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            SlotIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Key label ("1", "2", …)
    if (SlotKeyLabel)
    {
        SlotKeyLabel->SetText(FText::AsNumber(Index + 1));
    }

    // Active highlight
    if (ActiveHighlight)
    {
        ActiveHighlight->SetVisibility(bIsActive
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }

    OnSlotRefreshed();
}
