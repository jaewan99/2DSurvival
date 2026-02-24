// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ItemTooltipWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryTypes.h"
#include "Weapon/WeaponBase.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UItemTooltipWidget::SetItemDef(UItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (ItemNameText)
	{
		ItemNameText->SetText(ItemDef->DisplayName);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(ItemDef->Description);
	}

	if (StatsText)
	{
		const FString Stats = BuildStatsString(ItemDef);
		if (Stats.IsEmpty())
		{
			StatsText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			StatsText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			StatsText->SetText(FText::FromString(Stats));
		}
	}

	if (IconImage && ItemDef->Icon)
	{
		IconImage->SetBrushFromTexture(ItemDef->Icon);
		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else if (IconImage)
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FString UItemTooltipWidget::BuildStatsString(UItemDefinition* ItemDef) const
{
	if (!ItemDef) return FString();

	TArray<FString> Lines;

	// Weapon damage — read from CDO
	if (ItemDef->ItemCategory == EItemCategory::Weapon && ItemDef->WeaponActorClass)
	{
		const AWeaponBase* WeaponCDO = ItemDef->WeaponActorClass->GetDefaultObject<AWeaponBase>();
		if (WeaponCDO)
		{
			Lines.Add(FString::Printf(TEXT("Damage: %.0f"), WeaponCDO->BaseDamage));
		}
	}

	// Consumable heal amount
	if (ItemDef->ItemCategory == EItemCategory::Consumable && ItemDef->HealthRestoreAmount > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("Heals: +%.0f HP"), ItemDef->HealthRestoreAmount));
	}

	// Backpack bonus slots
	if (ItemDef->BonusSlots > 0)
	{
		Lines.Add(FString::Printf(TEXT("Bonus Slots: +%d"), ItemDef->BonusSlots));
	}

	// Stack size (only show if stackable)
	if (ItemDef->MaxStackSize > 1)
	{
		Lines.Add(FString::Printf(TEXT("Max Stack: %d"), ItemDef->MaxStackSize));
	}

	return FString::Join(Lines, TEXT("\n"));
}
