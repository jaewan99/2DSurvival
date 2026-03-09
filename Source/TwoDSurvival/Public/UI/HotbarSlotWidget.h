// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UItemDefinition;

/**
 * Base class for a single hotbar slot visual.
 * Create a Blueprint child (WBP_HotbarSlot) and design the slot however you want.
 *
 * Optionally add widgets with these exact names to get automatic data binding:
 *   - Image named "SlotIcon"        — filled with the item's icon texture
 *   - TextBlock named "SlotKeyLabel" — shows the slot's key number (1, 2, …)
 *   - Any UWidget named "ActiveHighlight" — shown only when this slot is active
 *
 * After C++ sets the data it calls OnSlotRefreshed() — override in Blueprint
 * for any additional custom visual logic.
 */
UCLASS()
class TWODSURVIVAL_API UHotbarSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // --- BindWidgetOptional: add these named widgets in BP if you want automatic binding ---

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> SlotIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SlotKeyLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> ActiveHighlight;

    // --- Current slot data (read-only, for Blueprint custom styling) ---

    UPROPERTY(BlueprintReadOnly, Category = "Hotbar")
    TObjectPtr<UItemDefinition> CurrentItemDef;

    UPROPERTY(BlueprintReadOnly, Category = "Hotbar")
    bool bIsActiveSlot = false;

    UPROPERTY(BlueprintReadOnly, Category = "Hotbar")
    int32 SlotIndex = 0;

    /**
     * Called by UHotbarWidget each time this slot's data changes.
     * Updates SlotIcon, SlotKeyLabel, and ActiveHighlight automatically if they exist.
     * Fires OnSlotRefreshed afterwards so Blueprint can do additional styling.
     */
    void SetSlotData(UItemDefinition* ItemDef, bool bIsActive, int32 Index);

    /**
     * Override in Blueprint to apply custom visual responses when slot data updates
     * (e.g. play an animation, change a material parameter).
     * CurrentItemDef / bIsActiveSlot / SlotIndex are already set when this fires.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar")
    void OnSlotRefreshed();
};
