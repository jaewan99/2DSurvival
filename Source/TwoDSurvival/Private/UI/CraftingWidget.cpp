// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CraftingWidget.h"
#include "UI/CraftingRecipeEntry.h"
#include "Kismet/GameplayStatics.h"
#include "Crafting/CraftingComponent.h"
#include "Crafting/CraftingRecipe.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Character/BaseCharacter.h"
#include "Components/SkillComponent.h"
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
	SkillComp     = Player->SkillComponent;

	if (!CraftingComp || !InventoryComp) return;

	SetTitle(FText::FromString(TEXT("Crafting")));

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

	const int32 CraftLevel = SkillComp ? SkillComp->GetLevel(ESkillType::Crafting) : 1;

	for (UCraftingRecipe* Recipe : CraftingComp->AllRecipes)
	{
		if (!Recipe) continue;

		// Hide recipes that require reading a book/schematic if the player hasn't learned them yet.
		if (Recipe->bRequiresLearning && !CraftingComp->IsRecipeLearned(Recipe->RecipeID))
			continue;

		const bool bLevelLocked = Recipe->MinCraftingLevel > CraftLevel;

		UItemDefinition* OutputDef = CraftingComp->FindItemDef(Recipe->OutputItemID);
		FText BaseName = OutputDef ? OutputDef->DisplayName : FText::FromName(Recipe->OutputItemID);

		FText RecipeName;
		if (bLevelLocked)
			RecipeName = FText::FromString(FString::Printf(TEXT("[Lv %d] %s"), Recipe->MinCraftingLevel, *BaseName.ToString()));
		else if (Recipe->IsUpgradeRecipe())
			RecipeName = FText::FromString(FString::Printf(TEXT("↑ %s"), *BaseName.ToString()));
		else
			RecipeName = BaseName;

		const bool bCanCraft = !bLevelLocked && CraftingComp->CanCraft(Recipe, InventoryComp);

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

		// For upgrade recipes show the base item at the top in cyan so it's
		// clearly distinguishable from the additional material requirements.
		if (SelectedRecipe->IsUpgradeRecipe())
		{
			UItemDefinition* BaseDef = CraftingComp->FindItemDef(SelectedRecipe->InputItemID);
			const int32 Have = InventoryComp ? InventoryComp->CountItemByID(SelectedRecipe->InputItemID) : 0;
			const bool bHasBase = Have >= 1;

			UHorizontalBox* BaseRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			UTextBlock* BaseLabelTB = WidgetTree->ConstructWidget<UTextBlock>();
			FString BaseLabel = FString::Printf(TEXT("↑  %s"),
				BaseDef ? *BaseDef->DisplayName.ToString() : *SelectedRecipe->InputItemID.ToString());
			BaseLabelTB->SetText(FText::FromString(BaseLabel));
			// Cyan tint to distinguish the base item from normal ingredients.
			BaseLabelTB->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 1.f)));
			UHorizontalBoxSlot* BaseLabelSlot = BaseRow->AddChildToHorizontalBox(BaseLabelTB);
			if (BaseLabelSlot)
			{
				FSlateChildSize Fill;
				Fill.SizeRule = ESlateSizeRule::Fill;
				BaseLabelSlot->SetSize(Fill);
			}

			UTextBlock* BaseCountTB = WidgetTree->ConstructWidget<UTextBlock>();
			BaseCountTB->SetText(FText::FromString(FString::Printf(TEXT("%d / 1"), Have)));
			BaseCountTB->SetColorAndOpacity(FSlateColor(bHasBase
				? FLinearColor(0.2f, 1.f, 0.2f)
				: FLinearColor(1.f, 0.3f, 0.3f)));
			BaseRow->AddChildToHorizontalBox(BaseCountTB);

			IngredientContainer->AddChildToVerticalBox(BaseRow);

			// Thin separator between base item and additional materials.
			if (!SelectedRecipe->Ingredients.IsEmpty())
			{
				UTextBlock* Sep = WidgetTree->ConstructWidget<UTextBlock>();
				Sep->SetText(FText::FromString(TEXT("──────────────────")));
				Sep->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f)));
				IngredientContainer->AddChildToVerticalBox(Sep);
			}
		}

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
	const int32 CraftLevel = SkillComp ? SkillComp->GetLevel(ESkillType::Crafting) : 1;
	const bool bLevelLocked = SelectedRecipe->MinCraftingLevel > CraftLevel;
	const bool bCanCraft = !bLevelLocked && CraftingComp->CanCraft(SelectedRecipe, InventoryComp);

	if (CraftButton) CraftButton->SetIsEnabled(bCanCraft);
	if (CraftStatusText)
	{
		FString Status;
		if (bLevelLocked)
			Status = FString::Printf(TEXT("Requires Crafting Lv %d"), SelectedRecipe->MinCraftingLevel);
		else
			Status = bCanCraft ? TEXT("Ready to craft") : TEXT("Missing ingredients");
		CraftStatusText->SetText(FText::FromString(Status));
	}
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
	{
		const bool bIsUpgrade = SelectedRecipe->IsUpgradeRecipe();
		CraftStatusText->SetText(FText::FromString(
			bSuccess ? (bIsUpgrade ? TEXT("Upgraded!") : TEXT("Crafted!")) : TEXT("Cannot craft")));
	}

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
