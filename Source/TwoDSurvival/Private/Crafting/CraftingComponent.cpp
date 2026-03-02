// Fill out your copyright notice in the Description page of Project Settings.

#include "Crafting/CraftingComponent.h"
#include "Crafting/CraftingRecipe.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"

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

bool UCraftingComponent::CanCraft(UCraftingRecipe* Recipe, UInventoryComponent* Inventory) const
{
	if (!Recipe || !Inventory) return false;

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

	// Consume ingredients
	for (const FIngredientEntry& Ingr : Recipe->Ingredients)
		Inventory->RemoveItemByID(Ingr.ItemID, Ingr.Count);

	// Add output
	Inventory->TryAddItem(OutputDef, Recipe->OutputCount);

	OnCraftingChanged.Broadcast();
	return true;
}
