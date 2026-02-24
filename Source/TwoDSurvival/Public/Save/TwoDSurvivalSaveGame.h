// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/HealthTypes.h"
#include "TwoDSurvivalSaveGame.generated.h"

/** Serialized representation of one inventory slot. */
USTRUCT()
struct FSavedInventorySlot
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;

	UPROPERTY()
	int32 Quantity = 0;
};

/** Serialized representation of one hotbar slot. */
USTRUCT()
struct FSavedHotbarSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;
};

/** Serialized representation of one body part's health. */
USTRUCT()
struct FSavedBodyPartHealth
{
	GENERATED_BODY()

	UPROPERTY()
	EBodyPart Part = EBodyPart::Body;

	UPROPERTY()
	float CurrentHealth = 0.f;

	UPROPERTY()
	float MaxHealth = 0.f;
};

/**
 * Save game object that persists player state:
 * inventory, hotbar, equipped weapon, body part health, and world position.
 */
UCLASS()
class TWODSURVIVAL_API UTwoDSurvivalSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSavedInventorySlot> InventorySlots;

	UPROPERTY()
	int32 BaseSlotCount = 0;

	UPROPERTY()
	TArray<FSavedHotbarSlot> HotbarSlots;

	UPROPERTY()
	int32 ActiveHotbarSlot = 0;

	UPROPERTY()
	FName EquippedWeaponItemID;

	UPROPERTY()
	TArray<FSavedBodyPartHealth> BodyPartHealthValues;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerRotation = FRotator::ZeroRotator;
};
