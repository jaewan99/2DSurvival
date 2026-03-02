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
};
