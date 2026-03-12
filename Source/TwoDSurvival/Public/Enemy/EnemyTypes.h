// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

class UItemDefinition;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle    UMETA(DisplayName = "Idle"),
	Patrol  UMETA(DisplayName = "Patrol"),
	Alert   UMETA(DisplayName = "Alert"),   // Investigating a heard noise
	Chase   UMETA(DisplayName = "Chase"),
	Attack  UMETA(DisplayName = "Attack"),
	Dead    UMETA(DisplayName = "Dead")
};

/** One entry in an enemy's loot table. */
USTRUCT(BlueprintType)
struct TWODSURVIVAL_API FLootEntry
{
	GENERATED_BODY()

	// Item to drop.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemDefinition> ItemDef;

	// Probability of this entry dropping (0 = never, 1 = always).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	// Minimum number of items dropped when this entry rolls successfully.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	// Maximum number of items dropped when this entry rolls successfully.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};
