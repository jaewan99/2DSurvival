// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarWidget.generated.h"

class UHorizontalBox;
class UHotbarComponent;
class UHotbarSlotWidget;

/**
 * Always-visible hotbar widget shown at the bottom of the screen.
 *
 * Blueprint child setup (WBP_HotbarWidget):
 *   1. Add a HorizontalBox named "SlotContainer" — slots are added here dynamically.
 *   2. Set SlotWidgetClass = WBP_HotbarSlot in the class defaults.
 *
 * The widget rebuilds its slot visuals automatically whenever the hotbar size changes
 * (e.g. when the player picks up or drops a belt or bag).
 */
UCLASS()
class TWODSURVIVAL_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UHorizontalBox* SlotContainer;

	/**
	 * The widget class used to create each slot visual.
	 * Assign WBP_HotbarSlot (your Blueprint child of UHotbarSlotWidget) here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hotbar")
	TSubclassOf<UHotbarSlotWidget> SlotWidgetClass;

	/** Rebuild slot widgets from scratch (called when slot count changes). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RebuildSlots();

	/** Refresh slot data without rebuilding (called when contents change). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshSlots();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnHotbarChanged();

	UPROPERTY()
	UHotbarComponent* CachedHotbarComp;

	UPROPERTY()
	TArray<UHotbarSlotWidget*> SlotWidgets;

	int32 LastKnownSlotCount = -1;
};
