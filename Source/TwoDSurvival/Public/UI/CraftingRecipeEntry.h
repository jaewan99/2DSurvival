// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingRecipeEntry.generated.h"

class UCraftingRecipe;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeEntryClicked, UCraftingRecipe*, Recipe);

/**
 * A single row in the crafting recipe list.
 * Blueprint child (WBP_CraftingRecipeEntry) needs:
 *   - Button named "EntryButton"
 *   - TextBlock named "RecipeNameText"
 *   - TextBlock named "CraftableText"
 */
UCLASS()
class TWODSURVIVAL_API UCraftingRecipeEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call after CreateWidget to set up the entry data.
	void Init(UCraftingRecipe* InRecipe, FText RecipeName, bool bCanCraft);

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnRecipeEntryClicked OnRecipeEntryClicked;

	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UCraftingRecipe> Recipe;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RecipeNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CraftableText;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
