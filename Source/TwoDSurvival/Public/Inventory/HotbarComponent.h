// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HotbarComponent.generated.h"

class UItemDefinition;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHotbarChanged);

/**
 * Manages a hotbar of quick-select item slots.
 * Each slot stores a UItemDefinition pointer. Selecting a slot auto-equips weapons
 * or auto-uses consumables via the owning BaseCharacter.
 * Validates slots against the linked inventory — clears slots for items no longer owned.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TWODSURVIVAL_API UHotbarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHotbarComponent();

	// Number of hotbar slots available. Configure in defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hotbar", meta = (ClampMin = 1))
	int32 HotbarSlotCount = 6;

	// Currently active (selected) hotbar slot index.
	UPROPERTY(BlueprintReadOnly, Category = "Hotbar")
	int32 ActiveSlotIndex = 0;

	// Broadcast whenever hotbar contents or active slot changes.
	UPROPERTY(BlueprintAssignable, Category = "Hotbar")
	FOnHotbarChanged OnHotbarChanged;

	// Assign an item definition to a hotbar slot.
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void AssignToHotbar(UItemDefinition* ItemDef, int32 HotbarSlotIndex);

	// Clear a hotbar slot.
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void ClearHotbarSlot(int32 HotbarSlotIndex);

	// Select a hotbar slot — equips weapon, uses consumable, or unequips if empty.
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void SelectSlot(int32 HotbarSlotIndex);

	// Cycle the active slot by Direction (+1 or -1), wrapping around.
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void CycleSlot(int32 Direction);

	// Get the item definition in the given hotbar slot.
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	UItemDefinition* GetHotbarSlotItem(int32 Index) const;

	// Get the item definition in the currently active slot.
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	UItemDefinition* GetActiveSlotItem() const;

	// The hotbar slot array — exposed for UI reading.
	UPROPERTY(BlueprintReadOnly, Category = "Hotbar")
	TArray<UItemDefinition*> HotbarSlots;

protected:
	virtual void BeginPlay() override;

private:
	// Cached reference to the owner's inventory component.
	UPROPERTY()
	UInventoryComponent* LinkedInventory;

	// Bound to LinkedInventory->OnInventoryChanged. Validates hotbar slots.
	UFUNCTION()
	void OnInventoryChanged();

	// Check if the linked inventory contains at least one of this item.
	bool InventoryContainsItem(UItemDefinition* ItemDef) const;

	// Find the first inventory slot index containing this item. Returns -1 if not found.
	int32 FindInventorySlotForItem(UItemDefinition* ItemDef) const;
};
