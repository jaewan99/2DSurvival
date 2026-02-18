# Inventory System Plan

## Overview

A slot-based inventory system. The player has a personal inventory. Containers in the world (chests, cabinets, etc.) also have inventories. Pressing E on a container (via the existing interaction system) opens the container's inventory UI alongside the player's inventory — items are moved by drag-and-drop between the two panels.

## Item Data

Each item is defined by a `UItemDefinition` data asset:

| Field | Type | Description |
|-------|------|-------------|
| `ItemID` | FName | Unique identifier |
| `DisplayName` | FText | Shown in UI |
| `Description` | FText | Tooltip text |
| `Icon` | UTexture2D | Inventory icon |
| `MaxStackSize` | int32 | Max units per slot (1 = not stackable) |
| `ItemCategory` | EItemCategory | Weapon / Tool / Consumable / Resource / Misc |
| `bCanBeEquipped` | bool | Whether it appears in the hotbar |

## Inventory Slot

`FInventorySlot` struct:
- `UItemDefinition* ItemDef` — null if the slot is empty
- `int32 Quantity`

## Architecture

### 1. `EItemCategory` (Enum)
- `Weapon`, `Tool`, `Consumable`, `Resource`, `Misc`

### 2. `UItemDefinition` (UDataAsset)
- All item metadata as listed above
- One data asset per item, created in the editor — no C++ subclasses per item

### 3. `FInventorySlot` (Struct — `USTRUCT(BlueprintType)`)
- `ItemDef` + `Quantity`
- Blueprint-exposed so UI widgets can read slots directly

### 4. `UInventoryComponent` (ActorComponent)
- Reusable — placed on **both** `ABaseCharacter` and container actors
- `TArray<FInventorySlot> Slots` — configurable size via `UPROPERTY(EditDefaultsOnly)`
- **Core methods:**
  - `TryAddItem(UItemDefinition*, int32 Quantity)` → bool (false if full)
  - `RemoveItem(int32 SlotIndex, int32 Quantity)`
  - `SwapSlots(int32 SlotA, UInventoryComponent* OtherComp, int32 SlotB)` — supports cross-inventory swaps
  - `GetSlot(int32 Index)` → `FInventorySlot`
- **Blueprint delegate:**
  - `OnInventoryChanged` (`UPROPERTY(BlueprintAssignable)`) — broadcast on any change

### 5. `UHotbarComponent` (ActorComponent on BaseCharacter only)
- `TArray<FInventorySlot> HotbarSlots` — 6 slots
- `int32 ActiveHotbarIndex`
- Methods: `SetActiveSlot(int32)`, `GetActiveItem()` → `FInventorySlot`
- Delegate: `OnHotbarChanged`

### 6. Container Actors (Blueprint)
- Plain Blueprint Actor with `UInventoryComponent` added as a component
- Set slot count and pre-populate slots in the Details panel
- Implements `IInteractable`:
  - `GetInteractionType()` → `Instant`
  - `GetInteractionPrompt()` → e.g., "Open Chest"
  - `OnInteract(ABaseCharacter*)` → calls `OpenContainerUI(this)` on the player's inventory widget
- Container state persists — items remain until the player takes them

### 7. `UInventoryWidget` (UMG — Blueprint)
- Two panels shown side-by-side when a container is open:
  - **Left**: player's `UInventoryComponent` slots
  - **Right**: container's `UInventoryComponent` slots
- When opened without a container (I key), only the player panel is shown
- Each slot widget shows icon + quantity badge
- Drag-and-drop between any slot in either panel using UMG's built-in DragDrop operations
  - On drop: call `SwapSlots` (or `TryAddItem` + `RemoveItem`) on the relevant components
- Bound to `OnInventoryChanged` on both components to refresh on change

### 8. `UHotbarWidget` (UMG — Blueprint)
- Always-visible row of 6 slots at the bottom of screen
- Highlights `ActiveHotbarIndex`
- Bound to `OnHotbarChanged`

## Input

| Action | Key | Binding |
|--------|-----|---------|
| `IA_ToggleInventory` | I | Toggle player inventory open/closed |
| `IA_HotbarSlot1–6` | 1–6 | Select hotbar slot directly |
| `IA_HotbarScroll` | Mouse Wheel | Cycle hotbar selection |

All bound in `SetupPlayerInputComponent` in C++. Container opening is triggered through `IInteractable::OnInteract` — no extra input action needed.

## Implementation Steps

1. Create `EItemCategory` enum, `UItemDefinition` data asset class, and `FInventorySlot` struct (`InventoryTypes.h`)
2. Create `UInventoryComponent` with slot array, add/remove/swap logic, and `OnInventoryChanged` delegate
3. Create `UHotbarComponent` with 6-slot array and active index logic
4. Add both components to `ABaseCharacter`
5. Bind inventory/hotbar input in `SetupPlayerInputComponent`
6. Create `UHotbarWidget` (UMG) — always visible, refresh on `OnHotbarChanged`
7. Create `UInventoryWidget` (UMG) — dual-panel, handles drag-and-drop, opens/closes on toggle or container interact
8. Wire UI in BP_BaseCharacter BeginPlay (Add to Viewport, initially hidden)
9. Create Blueprint container actor (e.g., `BP_Chest`) with `UInventoryComponent`, pre-populate slots
10. Create sample item data assets (e.g., `DA_Rock`, `DA_Stick`, `DA_Berry`)

## File Structure

```
Source/TwoDSurvival/
├── Public/
│   ├── Inventory/
│   │   ├── InventoryTypes.h          (EItemCategory, FInventorySlot)
│   │   ├── ItemDefinition.h          (UDataAsset)
│   │   ├── InventoryComponent.h      (ActorComponent — player + containers)
│   │   └── HotbarComponent.h         (ActorComponent — player only)
│   └── Character/
│       └── BaseCharacter.h           (add InventoryComponent, HotbarComponent, input actions)
├── Private/
│   ├── Inventory/
│   │   ├── ItemDefinition.cpp
│   │   ├── InventoryComponent.cpp
│   │   └── HotbarComponent.cpp
│   └── Character/
│       └── BaseCharacter.cpp
```

## Implementation Notes

### Cross-inventory drag and drop (UI)
- Each slot widget knows which `UInventoryComponent` it belongs to and its slot index
- `SwapSlots` on `UInventoryComponent` accepts a target component + target index — handles cross-component moves
- If source and target are the same component, it's a simple reorder
- If different components (player ↔ container), items are transferred

### Stacking on add
- `TryAddItem` first fills existing partial stacks of the same item before opening empty slots
- Returns false only if every slot is full and no partial stack exists

### Opening the container UI
- `OnInteract` on the container calls a function on the player character (or widget directly) passing its own `UInventoryComponent` reference
- The `UInventoryWidget` stores a `ContainerInventory` variable (nullable); if set, shows the right panel
- On close (I key or moving away), the reference is cleared and the right panel is hidden

### Inventory toggle + input mode
- On open: `SetInputMode(GameAndUI)`, show cursor
- On close: `SetInputMode(GameOnly)`, hide cursor
- Handled in BP_BaseCharacter or widget Open/Close functions

### Hotbar vs inventory
- Hotbar is separate from the main inventory — items must be explicitly moved there by dragging from inventory to hotbar slots in the UI
- `UHotbarWidget` drag targets accept drops from `UInventoryWidget` slots (cross-component swap)

### Key gotchas
- `FInventorySlot` must be `USTRUCT(BlueprintType)` with `UPROPERTY` on each field for UMG bindings
- `OnInventoryChanged` must be `UPROPERTY(BlueprintAssignable)` — a `FSimpleDynamicMulticastDelegate`
- Container actors do NOT need a C++ parent — plain Blueprint Actor with `UInventoryComponent` is sufficient
- Slot count on `UInventoryComponent` should be set in DefaultsOnly — not changed at runtime
