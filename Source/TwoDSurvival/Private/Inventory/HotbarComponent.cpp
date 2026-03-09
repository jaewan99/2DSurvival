// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryTypes.h"
#include "Character/BaseCharacter.h"
#include "World/FlashlightActor.h"

UHotbarComponent::UHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHotbarComponent::BeginPlay()
{
	Super::BeginPlay();

	LinkedInventory = GetOwner()->FindComponentByClass<UInventoryComponent>();
	if (LinkedInventory)
	{
		LinkedInventory->OnInventoryChanged.AddDynamic(this, &UHotbarComponent::OnInventoryChanged);
	}

	// Run initial hotbar size calculation based on whatever's already in inventory.
	OnInventoryChanged();
}

void UHotbarComponent::AssignToHotbar(UItemDefinition* ItemDef, int32 HotbarSlotIndex)
{
	if (!HotbarSlots.IsValidIndex(HotbarSlotIndex)) return;

	HotbarSlots[HotbarSlotIndex] = ItemDef;
	OnHotbarChanged.Broadcast();
}

void UHotbarComponent::ClearHotbarSlot(int32 HotbarSlotIndex)
{
	if (!HotbarSlots.IsValidIndex(HotbarSlotIndex)) return;

	HotbarSlots[HotbarSlotIndex] = nullptr;
	OnHotbarChanged.Broadcast();
}

void UHotbarComponent::SelectSlot(int32 HotbarSlotIndex)
{
	if (!HotbarSlots.IsValidIndex(HotbarSlotIndex)) return;

	ActiveSlotIndex = HotbarSlotIndex;

	UItemDefinition* ItemDef = HotbarSlots[HotbarSlotIndex];
	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!Character)
	{
		OnHotbarChanged.Broadcast();
		return;
	}

	if (!ItemDef || !InventoryContainsItem(ItemDef))
	{
		// Empty slot or item no longer in inventory — unequip
		Character->UnequipWeapon();
	}
	else if (ItemDef->ItemCategory == EItemCategory::Weapon)
	{
		int32 SlotIndex = FindInventorySlotForItem(ItemDef);
		if (SlotIndex >= 0)
		{
			Character->EquipItem(SlotIndex, LinkedInventory);
		}
	}
	else if (ItemDef->ItemCategory == EItemCategory::Consumable)
	{
		int32 SlotIndex = FindInventorySlotForItem(ItemDef);
		if (SlotIndex >= 0)
		{
			Character->UseItem(SlotIndex, LinkedInventory);
		}
	}
	else if (ItemDef->bIsFlashlight)
	{
		// Already equipped with this same flashlight → toggle on/off.
		if (Character->EquippedFlashlight
			&& Character->EquippedFlashlight->SourceItemDef == ItemDef)
		{
			Character->EquippedFlashlight->Toggle();
		}
		else
		{
			// Not yet equipped → equip it.
			int32 InvSlot = FindInventorySlotForItem(ItemDef);
			if (InvSlot >= 0)
			{
				Character->EquipItem(InvSlot, LinkedInventory);
			}
		}
	}

	OnHotbarChanged.Broadcast();
}

void UHotbarComponent::CycleSlot(int32 Direction)
{
	if (HotbarSlots.IsEmpty()) return;

	int32 NewIndex = ActiveSlotIndex + Direction;
	const int32 SlotCount = HotbarSlots.Num();
	if (NewIndex < 0) NewIndex = SlotCount - 1;
	else if (NewIndex >= SlotCount) NewIndex = 0;

	SelectSlot(NewIndex);
}

UItemDefinition* UHotbarComponent::GetHotbarSlotItem(int32 Index) const
{
	if (HotbarSlots.IsValidIndex(Index)) return HotbarSlots[Index];
	return nullptr;
}

UItemDefinition* UHotbarComponent::GetActiveSlotItem() const
{
	return GetHotbarSlotItem(ActiveSlotIndex);
}

void UHotbarComponent::OnInventoryChanged()
{
	// Validate: clear hotbar refs for items no longer in inventory.
	for (int32 i = 0; i < HotbarSlots.Num(); ++i)
	{
		if (HotbarSlots[i] && !InventoryContainsItem(HotbarSlots[i]))
		{
			HotbarSlots[i] = nullptr;
		}
	}

	// Auto-unequip flashlight if its source item left the inventory.
	ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (Character && Character->EquippedFlashlight
		&& Character->EquippedFlashlight->SourceItemDef
		&& !InventoryContainsItem(Character->EquippedFlashlight->SourceItemDef))
	{
		Character->UnequipFlashlight();
	}

	// Recalculate total hotbar bonus from all items in inventory.
	int32 NewTotal = HotbarSlotCount;
	if (LinkedInventory)
	{
		for (const FInventorySlot& Slot : LinkedInventory->Slots)
		{
			if (Slot.ItemDef && Slot.ItemDef->HotbarBonus > 0 && Slot.Quantity > 0)
			{
				NewTotal += Slot.ItemDef->HotbarBonus;
			}
		}
	}

	const int32 Current = HotbarSlots.Num();
	if (NewTotal > Current)       ExpandHotbar(NewTotal - Current);
	else if (NewTotal < Current)  ShrinkHotbar(Current - NewTotal);
	else                          OnHotbarChanged.Broadcast();
}

void UHotbarComponent::ExpandHotbar(int32 Count)
{
	if (Count <= 0) return;
	HotbarSlots.AddDefaulted(Count);
	OnHotbarChanged.Broadcast();
}

void UHotbarComponent::ShrinkHotbar(int32 Count)
{
	if (Count <= 0) return;
	const int32 NewCount = FMath::Max(0, HotbarSlots.Num() - Count);
	HotbarSlots.SetNum(NewCount);
	// Clamp active slot index to valid range.
	if (HotbarSlots.IsEmpty())             ActiveSlotIndex = 0;
	else if (ActiveSlotIndex >= NewCount)  ActiveSlotIndex = NewCount - 1;
	OnHotbarChanged.Broadcast();
}

bool UHotbarComponent::InventoryContainsItem(UItemDefinition* ItemDef) const
{
	if (!LinkedInventory || !ItemDef) return false;

	for (int32 i = 0; i < LinkedInventory->Slots.Num(); ++i)
	{
		if (LinkedInventory->Slots[i].ItemDef == ItemDef && LinkedInventory->Slots[i].Quantity > 0)
		{
			return true;
		}
	}
	return false;
}

int32 UHotbarComponent::FindInventorySlotForItem(UItemDefinition* ItemDef) const
{
	if (!LinkedInventory || !ItemDef) return -1;

	for (int32 i = 0; i < LinkedInventory->Slots.Num(); ++i)
	{
		if (LinkedInventory->Slots[i].ItemDef == ItemDef && LinkedInventory->Slots[i].Quantity > 0)
		{
			return i;
		}
	}
	return -1;
}
