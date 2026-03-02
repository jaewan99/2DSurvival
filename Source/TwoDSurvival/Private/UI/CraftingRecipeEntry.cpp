// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CraftingRecipeEntry.h"
#include "Crafting/CraftingRecipe.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCraftingRecipeEntry::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryButton)
		EntryButton->OnClicked.AddDynamic(this, &UCraftingRecipeEntry::HandleButtonClicked);
}

void UCraftingRecipeEntry::Init(UCraftingRecipe* InRecipe, FText RecipeName, bool bCanCraft)
{
	Recipe = InRecipe;

	if (RecipeNameText)
		RecipeNameText->SetText(RecipeName);

	if (CraftableText)
		CraftableText->SetText(FText::FromString(bCanCraft ? TEXT("[Can Craft]") : TEXT("[ ... ]")));

	// Color the craftable indicator
	if (CraftableText)
	{
		CraftableText->SetColorAndOpacity(bCanCraft
			? FSlateColor(FLinearColor(0.2f, 1.f, 0.2f))
			: FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
	}
}

void UCraftingRecipeEntry::HandleButtonClicked()
{
	OnRecipeEntryClicked.Broadcast(Recipe);
}
