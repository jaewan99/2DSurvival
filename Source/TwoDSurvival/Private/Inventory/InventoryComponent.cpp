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
	BaseSlotCount = SlotCount;

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
		// If this item provides bonus inventory slots, expand now.
		if (ItemDef->BonusSlots > 0)
		{
			ExpandSlots(ItemDef->BonusSlots);
		}

		OnInventoryChanged.Broadcast();
	}

	return Remaining == 0;
}

void UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || Quantity <= 0) return;

	FInventorySlot& Slot = Slots[SlotIndex];
	const int32 NewQuantity = FMath::Max(0, Slot.Quantity - Quantity);

	// If this item provides bonus slots and will be fully removed, try to shrink first.
	if (NewQuantity == 0 && Slot.ItemDef && Slot.ItemDef->BonusSlots > 0)
	{
		if (!ShrinkSlots(Slot.ItemDef->BonusSlots))
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot remove %s — bonus slots still contain items."),
				*Slot.ItemDef->DisplayName.ToString());
			return;
		}
	}

	Slot.Quantity = NewQuantity;
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

	FInventorySlot& SourceSlot = Slots[SlotA];
	FInventorySlot& DestSlot = OtherComp->Slots[SlotB];

	// If both slots have the same item type, try to stack them
	if (!SourceSlot.IsEmpty() && !DestSlot.IsEmpty() && SourceSlot.ItemDef == DestSlot.ItemDef)
	{
		const int32 MaxStack = SourceSlot.ItemDef->MaxStackSize;
		const int32 SpaceInDest = MaxStack - DestSlot.Quantity;

		if (SpaceInDest > 0)
		{
			// Transfer as much as possible from source to destination
			const int32 AmountToTransfer = FMath::Min(SourceSlot.Quantity, SpaceInDest);
			DestSlot.Quantity += AmountToTransfer;
			SourceSlot.Quantity -= AmountToTransfer;

			// Clear source slot if all items were transferred
			if (SourceSlot.Quantity <= 0)
			{
				SourceSlot.ItemDef = nullptr;
				SourceSlot.Quantity = 0;
			}

			OnInventoryChanged.Broadcast();
			if (OtherComp != this)
			{
				OtherComp->OnInventoryChanged.Broadcast();
			}
			return;
		}
		// If destination is already full (SpaceInDest <= 0), fall through to normal swap
	}

	// Normal swap for different items or empty slots
	FInventorySlot Temp = SourceSlot;
	SourceSlot = DestSlot;
	DestSlot = Temp;

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

void UInventoryComponent::ExpandSlots(int32 Amount)
{
	if (Amount <= 0) return;

	Slots.SetNum(Slots.Num() + Amount);
	SlotCount = Slots.Num();
	OnInventoryChanged.Broadcast();
}

bool UInventoryComponent::ShrinkSlots(int32 Amount)
{
	if (Amount <= 0) return true;

	const int32 CurrentCount = Slots.Num();
	if (Amount > CurrentCount) return false;

	// Check that the last N slots are empty.
	for (int32 i = CurrentCount - Amount; i < CurrentCount; ++i)
	{
		if (!Slots[i].IsEmpty())
		{
			return false;
		}
	}

	Slots.SetNum(CurrentCount - Amount);
	SlotCount = Slots.Num();
	OnInventoryChanged.Broadcast();
	return true;
}

bool UInventoryComponent::CanRemoveItem(int32 SlotIndex) const
{
	if (!Slots.IsValidIndex(SlotIndex)) return false;

	const FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.IsEmpty()) return true;
	if (!Slot.ItemDef || Slot.ItemDef->BonusSlots <= 0) return true;

	// If removing this item would shrink slots, check that trailing slots are empty.
	if (Slot.Quantity <= 1)
	{
		const int32 BonusSlots = Slot.ItemDef->BonusSlots;
		const int32 CurrentCount = Slots.Num();
		for (int32 i = CurrentCount - BonusSlots; i < CurrentCount; ++i)
		{
			if (i != SlotIndex && !Slots[i].IsEmpty())
			{
				return false;
			}
		}
	}

	return true;
}
