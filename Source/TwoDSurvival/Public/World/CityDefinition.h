// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CityDefinition.generated.h"

/**
 * Data asset that groups a set of streets into a named city.
 *
 * Streets declare their city by setting OwnerCity on UStreetDefinition.
 * Highways between cities have OwnerCity = null and bIsHighway = true.
 *
 * Create one DA_City_* per city. Set MapColor to a unique hue so each city
 * gets a distinct tinted region on the world map.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UCityDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique stable ID — never rename after first use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "City")
	FName CityID;

	/** Display name shown on the world map and in the HUD. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "City")
	FText CityName;

	/** Tint used for this city's region on the world map. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "City")
	FLinearColor MapColor = FLinearColor(0.2f, 0.5f, 0.9f, 1.f);

	/**
	 * Minimum number of streets to place from this city's pool on a new game.
	 * Clamped to the number of available streets if the pool is smaller.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation")
	int32 MinStreets = 4;

	/**
	 * Maximum number of streets to place from this city's pool on a new game.
	 * Actual count is a random value in [MinStreets, MaxStreets].
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation")
	int32 MaxStreets = 8;
};
