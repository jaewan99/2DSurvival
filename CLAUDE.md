# TwoDSurvival - Project Instructions

## Game Concept

- **Genre**: 2D Platformer Survival
- **Camera**: Side-scrolling (2D view) — camera faces the character from the side, movement is left/right
- **Perspective**: Orthographic-style side view using a Spring Arm + Camera setup on the BaseCharacter

## Progress Tracking

Whenever a task is completed, update `PROGRESS.md` at the project root by appending a new row to the table with:
- The next sequential task number
- A brief description of what was done
- Today's date
- Any relevant notes

Do NOT update PROGRESS.md automatically. Instead, ask the user for confirmation that the task is fully complete before adding it. A task may require multiple steps or iterations before it's done.

## C++ / Blueprint Conventions

- **BlueprintImplementableEvent** — can be overridden in Blueprint and called from C++, but is NOT reliably callable from OTHER Blueprints (e.g. a cabinet actor calling a function on the character).
- **BlueprintNativeEvent + BlueprintCallable** — use this whenever a function needs to be BOTH callable from Blueprint (including external Blueprints) AND overridable in Blueprint. Always declare the `virtual void FunctionName_Implementation()` in the header with an empty body.

## Weapon Equip Pattern

- `AWeaponBase` (C++) — base actor with `UStaticMeshComponent` (mesh) + `UBoxComponent` (hitbox, disabled by default) + `float BaseDamage` + `UItemDefinition* SourceItemDef`
- Each weapon item definition has `TSubclassOf<AWeaponBase> WeaponActorClass` — set to the Blueprint subclass (e.g. `BP_WeaponSword`)
- `EquipItem_Implementation` on `ABaseCharacter`: spawns `WeaponActorClass`, sets `SourceItemDef`, attaches to `WeaponSocketName` socket on character mesh
- `SourceItemDef` on the spawned actor is how the context menu identifies whether a slot's weapon is currently equipped (compare `EquippedWeapon->SourceItemDef == SlotData.ItemDef`)
- Socket name configured via `WeaponSocketName` (EditDefaultsOnly) on `BP_BaseCharacter` — create the socket in Skeleton editor

## Context Menu Pattern (UMG)

- Root is a `Size Box` (not Canvas Panel) — sized to content, positioned at cursor via `Set Position in Viewport`
- Dismissal: `bIsFocusable = true` on the widget + `Set Keyboard Focus (self)` in Event Construct + `On Focus Lost → Remove from Parent`
- Child buttons must have `Is Focusable = false` — otherwise clicking a button transfers focus first, firing `On Focus Lost` before `OnClicked`
- Mouse position: `Get Owning Player → Get Mouse Position On Viewport` + `Set Position in Viewport (bRemoveDPIScale: false)`
- Button visibility is category-driven — read `ItemCategory` and `bCanBeEquipped` from the slot's `ItemDef` in Event Construct

## UI Refresh Pattern (UMG Widgets)

When building inventory/UI systems with dynamic data:
- **Centralize refresh logic** — create ONE `RefreshAll()` function that updates all UI elements
- **Use delegate bindings** — bind `OnInventoryChanged` (or similar C++ delegates) directly to `RefreshAll`
- **Avoid scattered manual refresh calls** — let the delegate bindings handle updates automatically
- This prevents bugs where UI shows stale data and makes the system easier to reason about

Example: `WBP_InventoryWidget` has a single `RefreshAll()` that updates both player grid and container grid (if active). Both `InventoryComp->OnInventoryChanged` and `ContainerComp->OnInventoryChanged` are bound to call `RefreshAll()`. No manual refresh calls needed elsewhere.
