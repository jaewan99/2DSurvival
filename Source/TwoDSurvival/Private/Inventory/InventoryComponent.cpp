// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Pre-allocate all slots as empty.
	Slots.SetNum(SlotCount);

	for (const FInventoryStartingItem& Starting : StartingItems)
	{
		if (Starting.ItemDef)
		{
			TryAddItem(Starting.ItemDef, Starting.Quantity);
		}
	}
}

bool UInventoryComponent::TryAddItem(UItemDefinition* ItemDef, int32 Quantity)
{
	if (!ItemDef || Quantity <= 0) return false;

	int32 Remaining = Quantity;

	// Pass 1: fill existing partial stacks of the same item.
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) break;
		if (Slot.ItemDef != ItemDef) continue;

		const int32 Space = ItemDef->MaxStackSize - Slot.Quantity;
		if (Space <= 0) continue;

		const int32 Added = FMath::Min(Remaining, Space);
		Slot.Quantity += Added;
		Remaining -= Added;
	}

	// Pass 2: open empty slots.
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) break;
		if (!Slot.IsEmpty()) continue;

		const int32 Added = FMath::Min(Remaining, ItemDef->MaxStackSize);
		Slot.ItemDef = ItemDef;
		Slot.Quantity = Added;
		Remaining -= Added;
	}

	if (Remaining < Quantity)
	{
		OnInventoryChanged.Broadcast();
	}

	return Remaining == 0;
}

void UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || Quantity <= 0) return;

	FInventorySlot& Slot = Slots[SlotIndex];
	Slot.Quantity = FMath::Max(0, Slot.Quantity - Quantity);
	if (Slot.Quantity == 0)
	{
		Slot.ItemDef = nullptr;
	}

	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::SwapSlots(int32 SlotA, UInventoryComponent* OtherComp, int32 SlotB)
{
	if (!OtherComp) return;
	if (!Slots.IsValidIndex(SlotA) || !OtherComp->Slots.IsValidIndex(SlotB)) return;

	FInventorySlot Temp = Slots[SlotA];
	Slots[SlotA] = OtherComp->Slots[SlotB];
	OtherComp->Slots[SlotB] = Temp;

	OnInventoryChanged.Broadcast();
	if (OtherComp != this)
	{
		OtherComp->OnInventoryChanged.Broadcast();
	}
}

FInventorySlot UInventoryComponent::GetSlot(int32 Index) const
{
	if (Slots.IsValidIndex(Index)) return Slots[Index];
	return FInventorySlot();
}

bool UInventoryComponent::IsFull() const
{
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty()) return false;
		if (Slot.ItemDef && Slot.Quantity < Slot.ItemDef->MaxStackSize) return false;
	}
	return true;
}
