// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/HealthTypes.h"
#include "HealthHUDWidget.generated.h"

class UBodyPartRowWidget;
class UHealthComponent;
class UBorder;

/**
 * Health HUD widget displaying all 6 body parts with health bars.
 * Create a Blueprint child (WBP_HealthHUD) with 6x WBP_BodyPartRow widgets named:
 *   Row_Head, Row_Body, Row_LeftArm, Row_RightArm, Row_LeftLeg, Row_RightLeg.
 * Optionally add a Border named "TitleBar" for drag-to-move support.
 * Set bIsFocusable = false on the Blueprint so it doesn't steal input.
 */
UCLASS()
class TWODSURVIVAL_API UHealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_Head;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_Body;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_LeftArm;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_RightArm;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_LeftLeg;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBodyPartRowWidget* Row_RightLeg;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* TitleBar;

	/** Refresh all 6 body part rows from the HealthComponent. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshAll();

	/** Called by BaseCharacter after AddToViewport to sync the drag system with the widget's actual position. */
	void InitDragPosition(FVector2D ViewportPos);

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UFUNCTION()
	void OnHealthChanged(EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken);

	UPROPERTY()
	UHealthComponent* CachedHealthComp;

	bool bIsDragging = false;
	FVector2D LastMousePos;       // mouse position on last frame during drag
	FVector2D WidgetViewportPos;  // tracked viewport position (updated each drag frame)
};
