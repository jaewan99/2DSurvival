# TwoDSurvival - Progress Tracker

## Completed Steps

| # | Task | Date | Notes |
|---|------|------|-------|
| 1 | Initial project setup | 2026-02-14 | Created Unreal Engine project with default structure |
| 2 | GitHub Actions setup | 2026-02-14 | Added Claude Code workflow for issue handling (no code review) |
| 3 | 2D side-view camera setup | 2026-02-14 | SpringArm + Camera on BaseCharacter, side-scroll view, plane-constrained movement, smooth camera lag |
| 4 | 2D movement with character rotation | 2026-02-14 | MoveRight() BlueprintCallable function — moves left/right in world space, rotates character to face movement direction via SetControlRotation |
| 5 | Interaction system | 2026-02-15 | IInteractable interface, EInteractionType enum, UInteractionComponent on BaseCharacter. Proximity detection via dynamic sphere. Hold timer with movement lock, progress 0→1. Input (E key) bound in C++ via EnhancedInput (IA_Interact). UMG progress bar uses Add to Viewport with Get Player Character binding. |
| 6 | Inventory system | 2026-02-20 | UInventoryComponent (C++) reusable on player + containers. UItemDefinition data asset. FInventorySlot struct. WBP_InventoryWidget dual-panel (player + container). WBP_InventorySlot with drag-and-drop and item stacking. Centralized RefreshAll() bound to OnInventoryChanged delegate. BP_Cabinet container actor. UseItem BlueprintNativeEvent (Consumable, applies HealthRestoreAmount). IA_ToggleInventory (Tab) bound in C++. |
| 7 | Inventory context menu | 2026-02-24 | Right-click on WBP_InventorySlot → WBP_ContextMenu spawned at cursor. Category-driven buttons: Consumable → Use, bCanBeEquipped → Equip. Focus-based dismissal (bIsFocusable + Set Keyboard Focus + On Focus Lost). Child buttons Is Focusable = false to prevent premature focus loss. Position via Get Mouse Position On Viewport + Set Position in Viewport (bRemoveDPIScale: false). |
| 8 | Weapon equip system | 2026-02-24 | AWeaponBase (C++) — StaticMeshComponent + BoxComponent hitbox (disabled by default), BaseDamage, SourceItemDef reference. WeaponActorClass (TSubclassOf) added to UItemDefinition. EquipItem_Implementation: spawns weapon actor, attaches to WeaponSocketName socket on character mesh, stores EquippedWeapon ref. UnequipWeapon_Implementation: destroys actor, clears ref. Context menu shows Unequip instead of Equip when slot item matches EquippedWeapon.SourceItemDef. |
| 9 | Interaction mid-air fix | 2026-02-24 | Replaced DisableMovement() with bMovementLocked bool on ABaseCharacter. MoveRight checks flag and returns early. Gravity and physics still apply during hold interactions — character no longer hangs in mid-air when interacting while airborne. |
