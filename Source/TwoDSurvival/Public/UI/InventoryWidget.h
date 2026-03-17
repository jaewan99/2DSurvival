// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * C++ base class for WBP_InventoryWidget.
 *
 * Provides a draggable title bar — click and drag TitleBar to reposition the window.
 * TitleText shows a customisable title ("Inventory" by default, "Chest" when a container
 * is open — call SetTitle() from wherever the widget is opened).
 *
 * Blueprint child (WBP_InventoryWidget):
 *   - Reparent to UInventoryWidget.
 *   - Add a Border named "TitleBar" at the top of the layout — this is the drag handle.
 *   - Inside TitleBar, add a TextBlock named "TitleText" for the title label.
 *   - After AddToViewport / SetPositionInViewport, call InitDragPosition with the same position.
 *   - Set bIsFocusable = false so inventory doesn't steal keyboard input.
 *
 * No drag logic goes in Blueprint.
 */
UCLASS()
class TWODSURVIVAL_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The draggable handle — clicking anywhere inside this Border starts a drag. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* TitleBar;

	/** Title label inside TitleBar. Default text = "Inventory". */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TitleText;

	/**
	 * Sync the drag system with the widget's actual viewport position.
	 * Call this immediately after SetPositionInViewport so the first drag frame
	 * doesn't jump. Pass the same position you gave to SetPositionInViewport.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitDragPosition(FVector2D ViewportPos);

	/** Change the title text at runtime (e.g. "Inventory", "Chest", "Backpack"). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetTitle(const FText& NewTitle);

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	bool bIsDragging = false;
	FVector2D LastMousePos;
	FVector2D WidgetViewportPos;
};
