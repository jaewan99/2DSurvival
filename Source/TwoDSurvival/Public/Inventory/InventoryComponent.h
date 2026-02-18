// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * Reusable slot-based inventory component.
 * Attach to ABaseCharacter for the player inventory.
 * Attach to container actors (chests, cabinets) for their storage.
 *
 * - SlotCount is set in editor defaults — do not change at runtime.
 * - Bind OnInventoryChanged in Blueprint/UMG to refresh the inventory UI.
 * - SwapSlots supports cross-component transfers (player <-> container).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// Total number of slots. Configure in the component's DefaultsOnly details.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1))
	int32 SlotCount = 4;

	// Items added to the inventory on BeginPlay. Set in BP_BaseCharacter's InventoryComponent details.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TArray<FInventoryStartingItem> StartingItems;

	// All inventory slots — always has exactly SlotCount entries after BeginPlay.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Slots;

	// Broadcast whenever any slot is modified. Bind UMG widgets to this.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/**
	 * Try to add a quantity of an item.
	 * Fills existing partial stacks first, then opens empty slots.
	 * Returns true if ALL quantity was added.
	 * Returns false if the inventory was full (partial adds are still applied).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(UItemDefinition* ItemDef, int32 Quantity = 1);

	/** Remove a quantity from the given slot index. Clears the slot when quantity hits zero. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(int32 SlotIndex, int32 Quantity = 1);

	/**
	 * Swap (or merge) two slots.
	 * SlotA is on this component; SlotB is on OtherComp (may be the same component).
	 * Broadcasts OnInventoryChanged on both components when they differ.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapSlots(int32 SlotA, UInventoryComponent* OtherComp, int32 SlotB);

	/** Returns a copy of the slot at the given index, or an empty slot if out of range. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventorySlot GetSlot(int32 Index) const;

	/** Returns true if every slot is full (all stacks are at MaxStackSize). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

protected:
	virtual void BeginPlay() override;
};
