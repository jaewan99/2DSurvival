// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UCraftingRecipe;
class UInventoryComponent;
class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingChanged);

/**
 * Manages crafting for the owning character.
 * Auto-scans all UCraftingRecipe and UItemDefinition assets via AssetRegistry in BeginPlay.
 * Attach to ABaseCharacter — toggle the crafting UI with the C key.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingComponent();

	// Broadcast after a successful craft — UI binds to this for refresh.
	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingChanged OnCraftingChanged;

	// All recipes discovered via AssetRegistry scan in BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TArray<UCraftingRecipe*> AllRecipes;

	// True if Inventory holds all required ingredients for Recipe.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CanCraft(UCraftingRecipe* Recipe, UInventoryComponent* Inventory) const;

	// Consumes ingredients and adds the output item to Inventory.
	// Returns false if any ingredient is missing.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryCraft(UCraftingRecipe* Recipe, UInventoryComponent* Inventory);

	// Returns the UItemDefinition for the given ID (used by the crafting widget).
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	UItemDefinition* FindItemDef(FName ItemID) const;

protected:
	virtual void BeginPlay() override;

private:
	// ItemID → ItemDefinition map, built in BeginPlay.
	UPROPERTY()
	TMap<FName, UItemDefinition*> ItemDefMap;

	void ScanAssets();
};
