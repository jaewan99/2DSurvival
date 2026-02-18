// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UItemDefinition;

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Weapon     UMETA(DisplayName = "Weapon"),
	Tool       UMETA(DisplayName = "Tool"),
	Consumable UMETA(DisplayName = "Consumable"),
	Resource   UMETA(DisplayName = "Resource"),
	Misc       UMETA(DisplayName = "Misc"),
};

USTRUCT(BlueprintType)
struct TWODSURVIVAL_API FInventoryStartingItem
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDefinition> ItemDef = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = 1))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct TWODSURVIVAL_API FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDefinition> ItemDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	bool IsEmpty() const { return ItemDef == nullptr || Quantity <= 0; }
};
