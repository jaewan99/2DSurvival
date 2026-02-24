# TwoDSurvival - TODO

Items are loosely ordered by priority / dependency.

## Inventory & Equipment

- [ ] **Weapon grip animation** — Layered Blend per Bone in AnimBP; blend a grip pose on upper body when `EquippedWeapon != null`. Needs one grip pose asset per weapon type created in Skeleton editor.
- [x] **Hotbar system (C++)** — `UHotbarComponent` on BaseCharacter, 6 slots, active index, `OnHotbarChanged` delegate, SelectSlot/CycleSlot, input bindings in BaseCharacter.
- [x] **Unequip from context menu** — right-click equipped weapon → show Unequip button → calls `UnequipWeapon()`.
- [x] **Blueprint: add Unequip button to WBP_ContextMenu** — add `Btn_Unequip` (Collapsed by default) to Vertical Box; update Event Construct to show Unequip when `EquippedWeapon.SourceItemDef == SlotData.ItemDef`; `Btn_Unequip OnClicked` → `UnequipWeapon()` → `Remove from Parent`.
- [x] **Backpack (C++)** — `BonusSlots` on `UItemDefinition`, `ExpandSlots`/`ShrinkSlots`/`CanRemoveItem` on `UInventoryComponent`. Auto-expands on add, blocks removal if bonus slots occupied.
- [x] **Item tooltip (C++)** — `UItemTooltipWidget` with BindWidget text blocks, `SetItemDef()` populates name/desc/stats/icon.
- [x] **Persistence (C++)** — `UTwoDSurvivalSaveGame`, `SaveGame`/`LoadGame` on BaseCharacter, `SetBodyPartHealth` on HealthComponent. Serializes inventory, hotbar, health, position.

### Blueprint Wiring Steps (after C++ compile)

- [ ] **1. Hotbar Input Actions** — Create 8 Input Action assets in Content Browser: `IA_HotbarSlot1` through `IA_HotbarSlot6`, `IA_HotbarScrollUp`, `IA_HotbarScrollDown`. All Digital (bool), NO triggers on the asset.
- [ ] **2. IMC Bindings** — Open the Input Mapping Context and add: keys `1`–`6` → `IA_HotbarSlot1`–`IA_HotbarSlot6`, `Mouse Wheel Up` → `IA_HotbarScrollUp`, `Mouse Wheel Down` → `IA_HotbarScrollDown`.
- [ ] **3. BP_BaseCharacter: Assign Input Actions** — In `BP_BaseCharacter` Details, assign all 8 new IA assets to `IA_HotbarSlot1`–`IA_HotbarSlot6`, `IA_HotbarScrollUp`, `IA_HotbarScrollDown`.
- [ ] **4. BP_BaseCharacter: AllItemDefinitions** — In `BP_BaseCharacter` Details → Save category, populate the `AllItemDefinitions` array with every `UItemDefinition` data asset in the project (needed for save/load).
- [ ] **5. WBP_ItemTooltip** — Create a Widget Blueprint with parent class `UItemTooltipWidget`. Add Text Blocks named exactly: `ItemNameText`, `DescriptionText`, `StatsText`. Optionally add an Image named `IconImage`. Style as desired.
- [ ] **6. Hook Tooltip to Inventory Slots** — In `WBP_InventorySlot`: On Mouse Enter → create/show `WBP_ItemTooltip`, call `SetItemDef(SlotData.ItemDef)`, position near cursor. On Mouse Leave → remove tooltip from parent.
- [ ] **7. WBP_HotbarWidget** — Create a Widget Blueprint: horizontal box with 6 slot images/borders. Bind `OnHotbarChanged` delegate to refresh all 6 slots (read `HotbarComponent->GetHotbarSlotItem(i)`). Highlight the active slot (`ActiveSlotIndex`). Add to viewport in BP_BaseCharacter BeginPlay.
- [ ] **8. Context Menu: Assign to Hotbar** — In `WBP_ContextMenu`, add a `Btn_AssignHotbar` button (Is Focusable = false). Show for items with `bCanBeEquipped = true` or `ItemCategory == Consumable`. OnClicked → call `HotbarComponent->AssignToHotbar(ItemDef, ActiveSlotIndex)` → Remove from Parent.
- [ ] **9. Backpack Test Asset** — Create a new `UItemDefinition` data asset for a backpack item: set `BonusSlots = 4`, choose an icon. Add to a container or starting items for testing.
- [ ] **10. Save/Load UI** — Wire `SaveGame`/`LoadGame` to a key or pause menu button for testing (e.g. F5 to save, F9 to load).

## World / Interaction

- [x] **Door** — Blueprint Actor implementing `IInteractable` (Instant type). `OnInteract` toggles open/close via rotation or animation.

## Health

- [x] **Health system (C++)** — `UHealthComponent` with per-body-part pools (`EBodyPart`), `OnBodyPartDamaged` delegate, `OnDeath` delegate, movement/damage multipliers. Integrated into `ABaseCharacter` — leg damage slows movement, head/body at 0 fires death.
- [x] **Health HUD input (C++)** — `IA_ToggleHealthUI` property + `ToggleHealthUI` BlueprintNativeEvent on `ABaseCharacter`, bound in `SetupPlayerInputComponent`. Compiled clean.
- [ ] **Health HUD Blueprint** — `WBP_BodyPartRow` (label + progress bar + BROKEN text, `SetData` function with color logic) + `WBP_HealthHUD` (draggable panel, all 6 body part rows, delegate-driven `RefreshAll`). Full steps in `Source/plan/health-system.md`.
- [ ] **BP_BaseCharacter wiring** — assign `IA_ToggleHealthUI` asset, add H key to IMC, add `HealthHUDWidget` variable, override `ToggleHealthUI` to toggle widget.

## Combat

- [ ] **Attack input** — bind attack action (LMB or dedicated key) in C++.
- [ ] **Attack animation** — play attack montage when armed; enable `HitboxComponent` collision during the swing window, disable after.
- [ ] **Hit detection** — `HitboxComponent` overlap → apply `BaseDamage` to overlapping actors that implement a damageable interface.
- [ ] **Enemy / damageable interface** — `IDamageable` interface with `TakeDamage(float Amount)` for enemies and destructibles.

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
