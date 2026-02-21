// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "ItemDefinition.generated.h"

/**
 * Data asset that defines a single item type.
 * Create one asset per item in the editor (right-click Content Browser → Miscellaneous → Data Asset → UItemDefinition).
 * No C++ subclassing needed — all per-item data lives in the asset.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Unique identifier used to compare items in code.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	// Icon shown in the inventory UI slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	// Maximum number of this item per inventory slot (1 = not stackable).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemCategory ItemCategory = EItemCategory::Misc;

	// Amount of health restored when consumed (only relevant for Consumable category).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float HealthRestoreAmount = 0.f;

	// If true, this item can be placed in the hotbar.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanBeEquipped = false;
};