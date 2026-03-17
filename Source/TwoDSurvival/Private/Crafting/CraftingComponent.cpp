// Fill out your copyright notice in the Description page of Project Settings.

#include "Crafting/CraftingComponent.h"
#include "Crafting/CraftingRecipe.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/BaseCharacter.h"
#include "Components/SkillComponent.h"

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	ScanAssets();
}

void UCraftingComponent::ScanAssets()
{
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AR = ARM.Get();

	// Build ItemID → ItemDefinition map
	TArray<FAssetData> ItemAssets;
	AR.GetAssetsByClass(UItemDefinition::StaticClass()->GetClassPathName(), ItemAssets);
	for (const FAssetData& AD : ItemAssets)
	{
		UItemDefinition* Def = Cast<UItemDefinition>(AD.GetAsset());
		if (Def && !Def->ItemID.IsNone())
			ItemDefMap.Add(Def->ItemID, Def);
	}

	// Collect all crafting recipes
	TArray<FAssetData> RecipeAssets;
	AR.GetAssetsByClass(UCraftingRecipe::StaticClass()->GetClassPathName(), RecipeAssets);
	for (const FAssetData& AD : RecipeAssets)
	{
		UCraftingRecipe* Recipe = Cast<UCraftingRecipe>(AD.GetAsset());
		if (Recipe && !Recipe->RecipeID.IsNone())
			AllRecipes.Add(Recipe);
	}

	UE_LOG(LogTemp, Log, TEXT("CraftingComponent: %d recipes, %d item defs scanned."),
		AllRecipes.Num(), ItemDefMap.Num());
}

UItemDefinition* UCraftingComponent::FindItemDef(FName ItemID) const
{
	UItemDefinition* const* Found = ItemDefMap.Find(ItemID);
	return Found ? *Found : nullptr;
}

void UCraftingComponent::LearnRecipe(FName RecipeID)
{
	if (RecipeID.IsNone() || LearnedRecipeIDs.Contains(RecipeID)) return;
	LearnedRecipeIDs.Add(RecipeID);
	OnCraftingChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("CraftingComponent: learned recipe '%s'"), *RecipeID.ToString());
}

bool UCraftingComponent::IsRecipeLearned(FName RecipeID) const
{
	return LearnedRecipeIDs.Contains(RecipeID);
}

bool UCraftingComponent::CanCraft(UCraftingRecipe* Recipe, UInventoryComponent* Inventory) const
{
	if (!Recipe || !Inventory) return false;

	// Learning gate — recipe must be read from a book/schematic first.
	if (Recipe->bRequiresLearning && !LearnedRecipeIDs.Contains(Recipe->RecipeID))
		return false;

	// Crafting Lv2 gate — recipes with MinCraftingLevel > 1 require the skill level.
	if (Recipe->MinCraftingLevel > 1)
	{
		const ABaseCharacter* Owner = Cast<ABaseCharacter>(GetOwner());
		const int32 CraftLevel = (Owner && Owner->SkillComponent)
			? Owner->SkillComponent->GetLevel(ESkillType::Crafting) : 1;
		if (CraftLevel < Recipe->MinCraftingLevel) return false;
	}

	// Upgrade recipes also require 1× of the base item.
	if (Recipe->IsUpgradeRecipe())
	{
		if (Inventory->CountItemByID(Recipe->InputItemID) < 1)
			return false;
	}

	for (const FIngredientEntry& Ingr : Recipe->Ingredients)
	{
		if (Inventory->CountItemByID(Ingr.ItemID) < Ingr.Count)
			return false;
	}
	return true;
}

bool UCraftingComponent::TryCraft(UCraftingRecipe* Recipe, UInventoryComponent* Inventory)
{
	if (!CanCraft(Recipe, Inventory)) return false;

	UItemDefinition* OutputDef = FindItemDef(Recipe->OutputItemID);
	if (!OutputDef) return false;

	// Consume the base item for upgrade recipes.
	if (Recipe->IsUpgradeRecipe())
	{
		Inventory->RemoveItemByID(Recipe->InputItemID, 1);
	}

	// Consume ingredients
	for (const FIngredientEntry& Ingr : Recipe->Ingredients)
		Inventory->RemoveItemByID(Ingr.ItemID, Ingr.Count);

	// Crafting Lv3: 10% chance to produce double output.
	ABaseCharacter* Owner = Cast<ABaseCharacter>(GetOwner());
	USkillComponent* SC = Owner ? Owner->SkillComponent : nullptr;

	const int32 OutputCount = (SC && SC->RollDoubleOutput())
		? Recipe->OutputCount * 2 : Recipe->OutputCount;

	Inventory->TryAddItem(OutputDef, OutputCount);

	// Grant crafting XP.
	if (SC)
	{
		SC->AddXP(ESkillType::Crafting, SC->CraftingXPPerCraft);
	}

	OnCraftingChanged.Broadcast();
	return true;
}
