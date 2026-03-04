// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/NeedsComponent.h"
#include "NeedsWarningWidget.generated.h"

class UVerticalBox;
class UProgressBar;
class UWidget;
class USizeBox;

/**
 * Always-on HUD widget that shows Hunger, Thirst, and Fatigue as vertical progress bars.
 * Each icon collapses when the need is above 50% and expands when it drops below.
 * Create a Blueprint child (WBP_NeedsWarning) with a VerticalBox named "NeedsContainer".
 */
UCLASS()
class TWODSURVIVAL_API UNeedsWarningWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UVerticalBox* NeedsContainer;

	/** Rebuild all bars from current NeedsComponent state. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshAll();

protected:
	virtual void NativeConstruct() override;

private:
	/** Builds the three icon slots dynamically into NeedsContainer. */
	void BuildSlots();

	UFUNCTION()
	void OnNeedChanged(ENeedType NeedType, float CurrentValue, float MaxValue, bool bIsWarning);

	// Outer wrapper widget per need — collapsed when above threshold
	UPROPERTY()
	TArray<UWidget*> NeedIcons;

	// Progress bar per need
	UPROPERTY()
	TArray<UProgressBar*> NeedBars;

	// Cached NeedsComponent values after last refresh — indexed by ENeedType cast to int32
	float CachedValues[3] = { 100.f, 100.f, 100.f };

	float WarningThreshold = 50.f;
};
