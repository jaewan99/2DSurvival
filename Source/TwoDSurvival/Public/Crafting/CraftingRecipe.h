// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipe.generated.h"

/** One ingredient slot in a crafting recipe. */
USTRUCT(BlueprintType)
struct FIngredientEntry
{
	GENERATED_BODY()

	// ItemID matching a UItemDefinition asset.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	FName ItemID;

	// How many of this ingredient are consumed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = 1))
	int32 Count = 1;
};

/**
 * Data asset describing a single crafting recipe.
 * Create a DA_Recipe_* asset per craftable item.
 * UCraftingComponent auto-scans all UCraftingRecipe assets via AssetRegistry — no manual registration needed.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UCraftingRecipe : public UDataAsset
{
	GENERATED_BODY()

public:
	// Unique identifier for this recipe.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	FName RecipeID;

	// Items consumed when crafting.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TArray<FIngredientEntry> Ingredients;

	// ItemID of the item produced.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	FName OutputItemID;

	// How many of the output item are produced per craft.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = 1))
	int32 OutputCount = 1;

	/**
	 * Minimum Crafting skill level required to see and craft this recipe.
	 * 1 = available at start (default). 2 = unlocked at Crafting Lv2. 3 = Lv3 only.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = 1))
	int32 MinCraftingLevel = 1;

	/**
	 * Optional — the item that gets consumed as the upgrade base.
	 * When set, this becomes an UPGRADE recipe: the player must have 1 of this item
	 * in their inventory in addition to the normal Ingredients.
	 * On craft: 1× InputItemID is removed, Ingredients are consumed, OutputItemID is added.
	 * Leave as None for a normal crafting recipe.
	 *
	 * Example: InputItemID = "IronSword", Ingredients = [SteelIngot×2], Output = "SteelSword"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting|Upgrade")
	FName InputItemID;

	/** Returns true if this is an upgrade recipe (InputItemID is set). */
	bool IsUpgradeRecipe() const { return !InputItemID.IsNone(); }
};
