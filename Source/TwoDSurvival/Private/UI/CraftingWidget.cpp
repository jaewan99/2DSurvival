// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CraftingWidget.h"
#include "UI/CraftingRecipeEntry.h"
#include "Kismet/GameplayStatics.h"
#include "Crafting/CraftingComponent.h"
#include "Crafting/CraftingRecipe.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Character/BaseCharacter.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"

void UCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (!Player) return;

	CraftingComp  = Player->CraftingComponent;
	InventoryComp = Player->InventoryComponent;

	if (!CraftingComp || !InventoryComp) return;

	// Refresh craftability whenever inventory changes (picking up items, etc.)
	InventoryComp->OnInventoryChanged.AddDynamic(this, &UCraftingWidget::OnInventoryChanged);

	if (CraftButton)
		CraftButton->OnClicked.AddDynamic(this, &UCraftingWidget::OnCraftClicked);

	ClearDetail();
	BuildRecipeList();
}

// ---------------------------------------------------------
// Recipe list (left panel)
// ---------------------------------------------------------

void UCraftingWidget::BuildRecipeList()
{
	if (!RecipeScrollBox || !CraftingComp || !RecipeEntryClass) return;

	RecipeScrollBox->ClearChildren();

	for (UCraftingRecipe* Recipe : CraftingComp->AllRecipes)
	{
		if (!Recipe) continue;

		UItemDefinition* OutputDef = CraftingComp->FindItemDef(Recipe->OutputItemID);
		FText RecipeName = OutputDef ? OutputDef->DisplayName : FText::FromName(Recipe->OutputItemID);
		bool bCanCraft   = CraftingComp->CanCraft(Recipe, InventoryComp);

		UCraftingRecipeEntry* Entry = CreateWidget<UCraftingRecipeEntry>(GetOwningPlayer(), RecipeEntryClass);
		if (Entry)
		{
			Entry->Init(Recipe, RecipeName, bCanCraft);
			Entry->OnRecipeEntryClicked.AddDynamic(this, &UCraftingWidget::OnRecipeSelected);
			RecipeScrollBox->AddChild(Entry);
		}
	}
}

// ---------------------------------------------------------
// Detail panel (right panel)
// ---------------------------------------------------------

void UCraftingWidget::ClearDetail()
{
	if (OutputIcon)          OutputIcon->SetVisibility(ESlateVisibility::Hidden);
	if (OutputNameText)      OutputNameText->SetText(FText::FromString(TEXT("Select a recipe")));
	if (OutputCountText)     OutputCountText->SetText(FText::GetEmpty());
	if (IngredientContainer) IngredientContainer->ClearChildren();
	if (CraftButton)         CraftButton->SetIsEnabled(false);
	if (CraftStatusText)     CraftStatusText->SetText(FText::GetEmpty());
	SelectedRecipe = nullptr;
}

void UCraftingWidget::RefreshDetail()
{
	if (!SelectedRecipe)
	{
		ClearDetail();
		return;
	}

	UItemDefinition* OutputDef = CraftingComp->FindItemDef(SelectedRecipe->OutputItemID);

	// Output item
	if (OutputNameText)
		OutputNameText->SetText(OutputDef ? OutputDef->DisplayName : FText::FromName(SelectedRecipe->OutputItemID));

	if (OutputCountText)
		OutputCountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), SelectedRecipe->OutputCount)));

	if (OutputIcon)
	{
		if (OutputDef && OutputDef->Icon)
		{
			OutputIcon->SetBrushFromTexture(OutputDef->Icon);
			OutputIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			OutputIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// Ingredient rows
	if (IngredientContainer)
	{
		IngredientContainer->ClearChildren();

		for (const FIngredientEntry& Ingr : SelectedRecipe->Ingredients)
		{
			UItemDefinition* IngrDef = CraftingComp->FindItemDef(Ingr.ItemID);
			const int32 Have = InventoryComp ? InventoryComp->CountItemByID(Ingr.ItemID) : 0;
			const bool bEnough = Have >= Ingr.Count;

			// Row layout: [Name — fills remaining space]  [Have / Need]
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

			UTextBlock* NameTB = WidgetTree->ConstructWidget<UTextBlock>();
			NameTB->SetText(IngrDef ? IngrDef->DisplayName : FText::FromName(Ingr.ItemID));
			UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameTB);
			if (NameSlot)
			{
				FSlateChildSize Fill;
				Fill.SizeRule = ESlateSizeRule::Fill;
				NameSlot->SetSize(Fill);
			}

			UTextBlock* CountTB = WidgetTree->ConstructWidget<UTextBlock>();
			CountTB->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Have, Ingr.Count)));
			CountTB->SetColorAndOpacity(bEnough
				? FSlateColor(FLinearColor(0.2f, 1.f, 0.2f))
				: FSlateColor(FLinearColor(1.f, 0.3f, 0.3f)));
			Row->AddChildToHorizontalBox(CountTB);

			IngredientContainer->AddChildToVerticalBox(Row);
		}
	}

	// Craft button + status
	const bool bCanCraft = CraftingComp->CanCraft(SelectedRecipe, InventoryComp);
	if (CraftButton)     CraftButton->SetIsEnabled(bCanCraft);
	if (CraftStatusText) CraftStatusText->SetText(FText::FromString(
		bCanCraft ? TEXT("Ready to craft") : TEXT("Missing ingredients")));
}

// ---------------------------------------------------------
// Event handlers
// ---------------------------------------------------------

void UCraftingWidget::OnRecipeSelected(UCraftingRecipe* Recipe)
{
	SelectedRecipe = Recipe;
	RefreshDetail();
}

void UCraftingWidget::OnCraftClicked()
{
	if (!SelectedRecipe || !CraftingComp || !InventoryComp) return;

	const bool bSuccess = CraftingComp->TryCraft(SelectedRecipe, InventoryComp);

	if (CraftStatusText)
		CraftStatusText->SetText(FText::FromString(bSuccess ? TEXT("Crafted!") : TEXT("Cannot craft")));

	UGameplayStatics::PlaySound2D(this, bSuccess ? SFX_CraftSuccess : SFX_CraftFail);

	if (bSuccess)
	{
		BuildRecipeList(); // update craftable indicators
		RefreshDetail();   // update ingredient counts
	}
}

void UCraftingWidget::OnInventoryChanged()
{
	BuildRecipeList();
	RefreshDetail();
}
