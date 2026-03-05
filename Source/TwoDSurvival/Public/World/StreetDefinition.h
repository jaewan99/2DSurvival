// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StreetDefinition.generated.h"

class UBuildingDefinition;

UENUM(BlueprintType)
enum class EExitDirection : uint8
{
	Left  UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
	Up    UMETA(DisplayName = "Up"),
};

/**
 * Data asset describing one street: its sublevel asset, physical width, and connections to adjacent streets.
 * Exit connections live here — AStreetExit actors in the sublevel are spatial triggers only.
 * Create one DA_Street_* per playable street and link them via ExitLeft/Right/Up.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UStreetDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Unique stable ID — never rename after first use (used for save data keying).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FName StreetID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FText StreetName;

	// Width of this street in world units (cm). Used to place adjacent streets at the correct offset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	float StreetWidth = 4000.f;

	// The sublevel asset to stream in for this street.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	TSoftObjectPtr<UWorld> Level;

	// Connected streets — null means that exit is blocked (no arrow shown, trigger does nothing).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exits")
	TObjectPtr<UStreetDefinition> ExitLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exits")
	TObjectPtr<UStreetDefinition> ExitRight;

	// Up exits are intentional gates (door/ladder) — still uses an interactable actor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exits")
	TObjectPtr<UStreetDefinition> ExitUp;

	// If true, this definition represents a PCG-generated building interior (not a street).
	// UStreetManager will inject PCG parameters and teleport the player on entry/exit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	bool bIsPCGBuilding = false;

	// Building parameters passed into the PCG graph. Only used when bIsPCGBuilding = true.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building",
		meta = (EditCondition = "bIsPCGBuilding"))
	TObjectPtr<UBuildingDefinition> BuildingDefinition;

	// Returns the connected street for the given direction. Null = blocked.
	UStreetDefinition* GetExit(EExitDirection Direction) const;
};
