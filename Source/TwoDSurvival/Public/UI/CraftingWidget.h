// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingWidget.generated.h"

class UCraftingComponent;
class UInventoryComponent;
class UCraftingRecipe;
class UCraftingRecipeEntry;
class UScrollBox;
class UVerticalBox;
class UImage;
class UTextBlock;
class UButton;

/**
 * Main crafting UI widget.
 * Blueprint child (WBP_CraftingWidget) needs:
 *   Left panel:
 *     - ScrollBox named "RecipeScrollBox"
 *   Right panel:
 *     - Image named "OutputIcon"
 *     - TextBlock named "OutputNameText"
 *     - TextBlock named "OutputCountText"
 *     - VerticalBox named "IngredientContainer"
 *     - Button named "CraftButton"
 *     - TextBlock named "CraftStatusText"
 *
 * Assign WBP_CraftingRecipeEntry to RecipeEntryClass in the widget's Class Defaults.
 */
UCLASS()
class TWODSURVIVAL_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Assign WBP_CraftingRecipeEntry in the Blueprint child's Class Defaults.
	UPROPERTY(EditDefaultsOnly, Category = "Crafting")
	TSubclassOf<UCraftingRecipeEntry> RecipeEntryClass;

protected:
	// Left panel — scrollable recipe list
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> RecipeScrollBox;

	// Right panel — output item display
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> OutputIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OutputNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OutputCountText;

	// Right panel — ingredient list (rows built dynamically)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> IngredientContainer;

	// Right panel — craft action
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CraftButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CraftStatusText;

private:
	UPROPERTY()
	TObjectPtr<UCraftingComponent> CraftingComp;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComp;

	UPROPERTY()
	TObjectPtr<UCraftingRecipe> SelectedRecipe;

	// Rebuilds the left recipe list (called on open and after each craft).
	void BuildRecipeList();

	// Updates the right detail panel for SelectedRecipe.
	void RefreshDetail();

	// Clears the right panel to its empty/placeholder state.
	void ClearDetail();

	UFUNCTION()
	void OnRecipeSelected(UCraftingRecipe* Recipe);

	UFUNCTION()
	void OnCraftClicked();

	// Bound to InventoryComp->OnInventoryChanged — keeps craftability indicators current.
	UFUNCTION()
	void OnInventoryChanged();
};
