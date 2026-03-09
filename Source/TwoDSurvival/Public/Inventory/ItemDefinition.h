// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "Weapon/WeaponBase.h"
#include "ItemDefinition.generated.h"

class AFlashlightActor;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float HungerRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float ThirstRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float FatigueRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float MoodRestore = 0.f;

	/** Battery charge restored when consumed (only relevant for battery-type items). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable",
		meta = (EditCondition = "ItemCategory == EItemCategory::Consumable"))
	float BatteryRestoreAmount = 0.f;

	// If true, this item can be placed in the hotbar.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bCanBeEquipped = false;

	// Number of extra inventory slots this item provides when in inventory (e.g. backpack).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment", meta = (ClampMin = 0))
	int32 BonusSlots = 0;

	/** Number of hotbar quick-select slots this item grants when carried in inventory.
	 *  E.g. Belt = 1, Jacket = 1, Satchel = 2, Backpack = 2. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment", meta = (ClampMin = 0))
	int32 HotbarBonus = 0;

	// --- Flashlight ---

	/** If true, equipping this item spawns a AFlashlightActor attached to FlashlightSocket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	bool bIsFlashlight = false;

	/** The flashlight actor class to spawn when this item is equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment",
		meta = (EditCondition = "bIsFlashlight"))
	TSubclassOf<AFlashlightActor> FlashlightClass;

	// The weapon actor to spawn when this item is equipped. Only relevant for Weapon category.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Weapon",
		meta = (EditCondition = "ItemCategory == EItemCategory::Weapon"))
	TSubclassOf<AWeaponBase> WeaponActorClass;
};