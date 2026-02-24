// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryTypes.h"
#include "Character/BaseCharacter.h"

UHotbarComponent::UHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHotbarComponent::BeginPlay()
{
	Super::BeginPlay();

	HotbarSlots.SetNum(HotbarSlotCount);

	LinkedInventory = GetOwner()->FindComponentByClass<UInventoryComponent>();
	if (LinkedInventory)
	{
		LinkedInventory->OnInventoryChanged.AddDynamic(this, &UHotbarComponent::OnInventoryChanged);
	}
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

	OnHotbarChanged.Broadcast();
}

void UHotbarComponent::CycleSlot(int32 Direction)
{
	int32 NewIndex = ActiveSlotIndex + Direction;
	if (NewIndex < 0) NewIndex = HotbarSlotCount - 1;
	else if (NewIndex >= HotbarSlotCount) NewIndex = 0;

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
	bool bChanged = false;
	for (int32 i = 0; i < HotbarSlots.Num(); ++i)
	{
		if (HotbarSlots[i] && !InventoryContainsItem(HotbarSlots[i]))
		{
			HotbarSlots[i] = nullptr;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnHotbarChanged.Broadcast();
	}
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
