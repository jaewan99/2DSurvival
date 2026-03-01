# TwoDSurvival - TODO

Items are loosely ordered by priority / dependency.

## Inventory & Equipment

- [ ] **Weapon grip animation** — Layered Blend per Bone in AnimBP; blend a grip pose on upper body when `EquippedWeapon != null`. Needs one grip pose asset per weapon type created in Skeleton editor.
- [x] **Hotbar system (C++)** — `UHotbarComponent` on BaseCharacter, 6 slots, active index, `OnHotbarChanged` delegate, SelectSlot/CycleSlot, input bindings in BaseCharacter.
- [x] **Hotbar widget (C++)** — `UHotbarWidget` builds 6 slots dynamically in NativeConstruct (border + icon + key label). Gold highlight on active slot. Always visible at bottom of screen.
- [x] **Unequip from context menu** — right-click equipped weapon → show Unequip button → calls `UnequipWeapon()`.
- [x] **Blueprint: add Unequip button to WBP_ContextMenu** — add `Btn_Unequip` (Collapsed by default) to Vertical Box; update Event Construct to show Unequip when `EquippedWeapon.SourceItemDef == SlotData.ItemDef`; `Btn_Unequip OnClicked` → `UnequipWeapon()` → `Remove from Parent`.
- [x] **Backpack (C++)** — `BonusSlots` on `UItemDefinition`, `ExpandSlots`/`ShrinkSlots`/`CanRemoveItem` on `UInventoryComponent`. Auto-expands on add, blocks removal if bonus slots occupied.
- [x] **Item tooltip (C++)** — `UItemTooltipWidget` with BindWidget text blocks, `SetItemDef()` populates name/desc/stats/icon.
- [x] **Persistence (C++)** — `UTwoDSurvivalSaveGame`, `SaveGame`/`LoadGame` on BaseCharacter, `SetBodyPartHealth` on HealthComponent. Serializes inventory, hotbar, health, position. F5 = save, F9 = load (programmatic input).

### Remaining Blueprint Wiring

- [x] **WBP_ItemTooltip** — Create Widget Blueprint (parent: `UItemTooltipWidget`). Add TextBlocks named `ItemNameText`, `DescriptionText`, `StatsText`. Optionally add Image named `IconImage`. Style as desired.
- [x] **Hook tooltip to inventory slots** — In `WBP_InventorySlot`: Mouse Enter → create `WBP_ItemTooltip`, call `SetItemDef(SlotData.ItemDef)`, position near cursor. Mouse Leave → remove from parent. Right-click removes tooltip before opening context menu.
- [x] **Context menu: Assign to Hotbar** — Add `Btn_AssignHotbar` button (Is Focusable = false) to `WBP_ContextMenu`. Show for Weapon/Consumable. OnClicked → `HotbarComponent->AssignToHotbar(ItemDef, ActiveSlotIndex)` → Remove from Parent.
- [x] **Backpack test asset** — Created `DA_Backpack` `UItemDefinition` with `BonusSlots = 4`.

## World / Interaction

- [x] **Door** — Blueprint Actor implementing `IInteractable` (Instant type). `OnInteract` toggles open/close via rotation or animation.

## Health

- [x] **Health system (C++)** — `UHealthComponent` with per-body-part pools (`EBodyPart`), `OnBodyPartDamaged` delegate, `OnDeath` delegate, movement/damage multipliers. Integrated into `ABaseCharacter`.
- [x] **Health HUD (C++ + Blueprint)** — `UBodyPartRowWidget` + `UHealthHUDWidget`. Draggable panel, 6 color-coded body part rows, delegate-driven refresh. Toggle with H key. `FInputModeGameAndUI` on open.

## Combat

- [x] **Attack input** — `IA_Attack` (LMB) created programmatically in `CreateInputActions()`, bound to `OnAttackPressed` in `SetupPlayerInputComponent`.
- [x] **Attack animation** — `PlayAnimMontage` called from C++ for both armed (weapon's `AttackMontage`) and unarmed (`UnarmedAttackMontage`). Montage assets assigned in BP Details panel.
- [x] **Hitbox enable/disable** — `AWeaponBase::BeginAttack()` enables `HitboxComponent` collision + starts `SwingWindowDuration` timer → `EndAttack()`. `EndAttack()` is public so `AnimNotify_EndAttack` can also close it.
- [x] **Hit detection** — `OnHitboxOverlap` on `AWeaponBase` checks `IDamageable`, prevents multi-hit via `HitActorsThisSwing` set, calls `Execute_TakeMeleeDamage`.
- [x] **Unarmed hit detection** — delayed sphere sweep in `PerformUnarmedHit()`, hits first `IDamageable` actor in range.
- [x] **IDamageable interface** — `TakeMeleeDamage(Amount, DamageSource)` — BlueprintNativeEvent + BlueprintCallable.
- [x] **Player implements IDamageable** — `ABaseCharacter` implements `TakeMeleeDamage_Implementation`; damages the `Body` part via `HealthComponent->ApplyDamage`.
- [x] **Block attack during UI** — `OnAttackPressed` returns early if `PC->bShowMouseCursor` is true (inventory / health HUD open).
- [x] **AnimNotify_BeginAttack** — C++ `UAnimNotify` subclass; casts owner to `ABaseCharacter`, calls `EquippedWeapon->BeginAttack(DamageMultiplier)` at the precise swing frame.
- [x] **AnimNotify_EndAttack** — C++ `UAnimNotify` subclass; calls `EquippedWeapon->EndAttack()` to close the hitbox early if the notify fires before the timer.
- [x] **Player death handler** — `HandlePlayerDeath()` bound to `HealthComponent->OnDeath`; disables input, plays `DeathMontage`, fires `OnPlayerDied` BlueprintImplementableEvent.
- [x] **AnimBP IK disable** — `ABP_Player` reads `bIsAttacking` from character via `Event Blueprint Update Animation` cast. Wired to IK node Alpha pins. Confirmed working for both punch and weapon swing.
- [x] **AnimNotifies on weapon montage** — `AnimNotify_BeginAttack` and `AnimNotify_EndAttack` added to weapon attack montage at the correct swing frames.
- [ ] **Death handling** — When Head or Body health reaches 0:
  - C++ fires `OnPlayerDied` BlueprintImplementableEvent and calls `DisableInput` (already done)
  - [ ] Assign `DeathMontage` asset on `BP_BaseCharacter` Details panel
  - [ ] Override `OnPlayerDied` in `BP_BaseCharacter` — show death/respawn widget, optionally re-enable input after delay for respawn
  - [ ] Decide respawn behaviour: reload last save, respawn at origin, or show main menu

## Enemy

- [ ] **Base enemy** — `AEnemyBase` (C++) with `UHealthComponent`, movement (patrol / chase states), and `IDamageable` implementation. Blueprint child `BP_Enemy_Base` for asset assignment.
- [ ] **AI states** — Patrol (walk between waypoints), Detect (line-of-sight or proximity trigger), Chase (move toward player), Attack (enter range → trigger attack), Flee (optional, low health).
- [ ] **Enemy attack** — melee swing with hitbox overlap that calls `HealthComponent->ApplyDamage` on the player's body part(s).
- [ ] **Enemy health bar** — world-space widget above enemy showing a simple health bar; hide when at full health, show briefly on damage.
- [ ] **Drop loot** — on death, spawn item actors from a configurable loot table (array of `UItemDefinition` + weight).
- [ ] **Enemy spawner** — Blueprint Actor with spawn radius, max count, respawn delay; spawns `BP_Enemy` variants in the world.

## Crafting

- [ ] **Recipe data asset** — `UCraftingRecipe` (UDataAsset): array of `FIngredient` (ItemDef + quantity), output ItemDef + quantity, optional required tool/station.
- [ ] **Crafting component** — `UCraftingComponent` (C++) on the player: `CanCraft(Recipe)` checks inventory, `Craft(Recipe)` consumes ingredients and calls `InventoryComponent->TryAddItem`.
- [ ] **Crafting UI** — `WBP_CraftingWidget`: scrollable recipe list on the left, selected recipe detail (ingredients, output) on the right, Craft button (greyed out if missing ingredients). Toggle with a key (e.g. C).
- [ ] **Item upgrading** — extend `UCraftingRecipe` with an optional `InputItemDef` (item being upgraded) + `OutputItemDef` (upgraded result). `UCraftingComponent->Upgrade(Recipe, SlotIndex)` removes the base item and adds the upgraded one.
- [ ] **Crafting station** — Blueprint Actor (workbench, forge, etc.) implementing `IInteractable`; `OnInteract` opens `WBP_CraftingWidget` filtered to recipes that require that station.

## Moods / Needs

Inspired by Project Zomboid — needs decay over time and affect gameplay stats.

- [ ] **Needs component** — `UNeedsComponent` (C++) on the player. Tracks float values (0–100) for each need. Decays via `Tick` at configurable rates. Broadcasts `OnNeedChanged(ENeed, float NewValue)` delegate.
- [ ] **Need types** — initial set:
  - `Hunger` — low hunger reduces stamina regen / movement speed at critical level
  - `Thirst` — faster decay than hunger; critical level applies damage over time to Body
  - `Fatigue` — increases over time without rest; slows movement and reduces attack damage
  - `Mood` / `Depression` — degrades when other needs are critical, when isolated, or on death; low mood reduces XP gain, dims screen tint slightly
- [ ] **Need effects** — `UNeedsComponent` fires threshold callbacks (e.g. at 25% and 0%) that `ABaseCharacter` listens to and applies stat modifiers (walk speed, damage multiplier, DoT).
- [ ] **Satisfying needs** — consuming food items reduces Hunger (extend `UseItem`); water items reduce Thirst; sleeping (interact with bed) restores Fatigue over time.
- [ ] **Needs HUD** — `WBP_NeedsHUD`: small always-visible (or H-toggled) panel showing icons + bars for each need. Color shifts yellow → red as value drops. Urgent needs pulse or show warning text.
- [ ] **Mood modifiers** — positive events (eating a hot meal, completing a craft, finding rare loot) give a temporary Mood boost; negative events (taking damage, staying in darkness, witnessing death) reduce it.

## House Customization

Player can place, move, and remove furniture/decorations inside their base.

- [ ] **Placeable item data** — extend `UItemDefinition` with `bIsPlaceable` bool + `TSubclassOf<APlaceableActor> PlaceableClass` (EditCondition gated).
- [ ] **Placeable actor base** — `APlaceableActor` (C++): static mesh, collision, a `PlacementID` (FGuid for save/load). Blueprint children for each furniture piece.
- [ ] **Placement mode** — dedicated input (e.g. RMB while holding a placeable item) enters placement mode: ghost mesh follows cursor clamped to a grid, green/red tint based on overlap. Confirm (LMB) spawns the actor; cancel (Esc) exits.
- [ ] **Move / remove** — interact with a placed actor to get a context menu: Move (re-enter placement mode with that actor) or Pick Up (destroys actor, returns item to inventory).
- [ ] **Room zones** — optional axis-aligned box volumes that define valid placement areas; placement outside a zone is rejected. Allows "only place furniture indoors".
- [ ] **Persistence** — save placed actor transforms + class references in `USaveGame`; respawn them on load.
