// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BodyPartRowWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * A single row in the Health HUD showing one body part's name, health bar, and broken indicator.
 * Create a Blueprint child (WBP_BodyPartRow) and add widgets named:
 *   PartLabel (TextBlock), HealthBar (ProgressBar), BrokenText (TextBlock, optional).
 */
UCLASS()
class TWODSURVIVAL_API UBodyPartRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* PartLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* BrokenText;

	/** Set the body part name and health percentage (0–1). Updates bar color and broken indicator. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetData(FName PartName, float Percent);
};
