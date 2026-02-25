// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HotbarWidget.generated.h"

class UHorizontalBox;
class UBorder;
class UImage;
class UHotbarComponent;
class UTextBlock;

/**
 * Hotbar widget that displays quick-select item slots at the bottom of the screen.
 * Creates slot visuals dynamically in NativeConstruct — no per-slot BindWidget needed.
 * Create a Blueprint child (WBP_HotbarWidget) with a HorizontalBox named "SlotContainer".
 */
UCLASS()
class TWODSURVIVAL_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UHorizontalBox* SlotContainer;

	/** Refresh all slot icons and active highlight from the HotbarComponent. */
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
	TArray<UBorder*> SlotBorders;

	UPROPERTY()
	TArray<UImage*> SlotIcons;

	UPROPERTY()
	TArray<UTextBlock*> SlotKeyLabels;
};
