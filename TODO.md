# TwoDSurvival - TODO

Items are loosely ordered by priority / dependency.

## Inventory & Equipment

- [ ] **Weapon grip animation** — Layered Blend per Bone in AnimBP; blend a grip pose on upper body when `EquippedWeapon != null`. Needs one grip pose asset per weapon type created in Skeleton editor.
- [ ] **Hotbar system** — `UHotbarComponent` (C++): 6 slots, active index, `OnHotbarChanged` delegate. `WBP_HotbarWidget`: always-visible row at bottom of screen, highlights active slot. Wire up `EquipItem` to move item to first free hotbar slot. Input: mouse wheel + 1–6 keys bound in C++.
- [x] **Unequip from context menu** — right-click equipped weapon → show Unequip button → calls `UnequipWeapon()`.
- [ ] **Blueprint: add Unequip button to WBP_ContextMenu** — add `Btn_Unequip` (Collapsed by default) to Vertical Box; update Event Construct to show Unequip when `EquippedWeapon.SourceItemDef == SlotData.ItemDef`; `Btn_Unequip OnClicked` → `UnequipWeapon()` → `Remove from Parent`.
- [ ] **Backpack** — `int32 BonusSlots` on `UItemDefinition`, `ExpandSlots`/`ShrinkSlots` on `UInventoryComponent`. Equip backpack → expands player inventory. Needs hotbar done first (same Equip flow).
- [ ] **Item tooltip** — hover over an inventory slot → show item name + description in a small floating widget.
- [ ] **Persistence** — save/load inventory and equipped weapon state between sessions using `USaveGame`.

## World / Interaction

- [ ] **Door** — Blueprint Actor implementing `IInteractable` (Instant type). `OnInteract` toggles open/close via rotation or animation.

## Combat

- [ ] **Attack input** — bind attack action (LMB or dedicated key) in C++.
- [ ] **Attack animation** — play attack montage when armed; enable `HitboxComponent` collision during the swing window, disable after.
- [ ] **Hit detection** — `HitboxComponent` overlap → apply `BaseDamage` to overlapping actors that implement a damageable interface.
- [ ] **Enemy / damageable interface** — `IDamageable` interface with `TakeDamage(float Amount)` for enemies and destructibles.
