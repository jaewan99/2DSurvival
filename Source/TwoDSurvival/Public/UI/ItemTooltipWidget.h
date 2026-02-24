// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltipWidget.generated.h"

class UItemDefinition;
class UTextBlock;
class UImage;

/**
 * Tooltip widget that displays item information.
 * Create a Blueprint child (WBP_ItemTooltip) and add widgets named:
 *   ItemNameText, DescriptionText, StatsText (optional), IconImage (optional).
 */
UCLASS()
class TWODSURVIVAL_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* ItemNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* DescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* StatsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* IconImage;

	/** Populate all fields from the given item definition. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetItemDef(UItemDefinition* ItemDef);

private:
	/** Build a multi-line stats string based on item category and properties. */
	FString BuildStatsString(UItemDefinition* ItemDef) const;
};
