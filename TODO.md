# TwoDSurvival - TODO

Items are loosely ordered by priority / dependency.

## Inventory & Equipment

- [ ] **Weapon grip animation** — Layered Blend per Bone in AnimBP; blend a grip pose on upper body when `EquippedWeapon != null`. Needs one grip pose asset per weapon type created in Skeleton editor.
- [ ] **Needs speed penalty — animation hookup** — `MaxWalkSpeed` is correctly reduced in C++ (via `RecalculateMovementSpeed`) but the visual movement speed depends on how the AnimBP is set up:
  - If walk/run animations use **root motion**, MaxWalkSpeed alone won't change visual speed. Disable root motion on locomotion animations and let `AddMovementInput` + `MaxWalkSpeed` drive position.
  - If root motion is already off, add `MovementSpeedMultiplier` (`BlueprintReadOnly`) to `ABaseCharacter`, set it alongside `MaxWalkSpeed` in `RecalculateMovementSpeed`, then read it in `ABP_Player → Event Blueprint Update Animation` and feed it into the locomotion blend space play rate.
  - Either way, the AnimBP should drive blend space from `GetVelocity().Size()` so animation naturally reflects the capped speed.
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

- [x] **Base enemy** — `AEnemyBase` (C++) with `UHealthComponent`, C++ state machine (Idle/Patrol/Chase/Attack/Dead), `IDamageable` implementation. `BP_EnemyBase` Blueprint child for mesh/AnimBP/loot assignment.
- [x] **AI states** — Idle (wait then patrol), Patrol (wander within PatrolRange of spawn), Chase (move toward player on X axis), Attack (swing + cooldown), Dead (loot drop + delayed destroy). Aggro via sphere distance; lose-aggro hysteresis via LoseAggroMultiplier.
- [x] **Enemy attack** — `UBoxComponent` MeleeHitbox enabled per swing (BeginMeleeAttack/EndMeleeAttack pattern mirrors AWeaponBase). `HitActorsThisSwing` set prevents multi-hit. Calls `Execute_TakeMeleeDamage` on IDamageable targets.
- [x] **Post-attack stand-still** — `PostAttackStandStillDuration` (EditDefaultsOnly) locks movement and re-attack for an extra window after `AttackCooldown`. Prevents sliding when transitioning back to walk.
- [x] **Rotation lock during attack** — enemy facing is frozen while `bIsAttacking` is true; player can jump over mid-swing without the enemy tracking them.
- [x] **Enemy health bar** — `UEnemyHealthBarWidget` (C++ UUserWidget, BindWidget ProgressBar). Attached via `UWidgetComponent` in Screen space above enemy's head. Hidden until first damage hit. Color-coded green/yellow/red.
- [x] **Drop loot** — `FLootEntry` struct (ItemDef, DropChance, MinCount, MaxCount). `LootTable` TArray on AEnemyBase (EditDefaultsOnly). On death: roll per entry, spawn `AWorldItem` actors with random offset.
- [x] **World item pickup** — `AWorldItem` (AActor + IInteractable). Instant interaction type. Press E → `TryAddItem` on player InventoryComponent → self-destruct on success.
- [ ] **Enemy spawner** — Blueprint Actor with spawn radius, max count, respawn delay; spawns `BP_EnemyBase` variants in the world.

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

## World Map

Each "map" is a street level. At the end of a street the player can transition to up to 3 adjacent streets (left/right/up). Blocked exits are hidden or greyed out. Streets are loaded/unloaded as the player moves between them.

- [x] **Street data asset** — `UStreetDefinition` (UDataAsset): street name, background/tileset reference, up to 3 exit slots (`EExitDirection`: Left, Right, Up), each exit holds a `TSoftObjectPtr<UStreetDefinition>` to the destination street + optional lock condition.
- [x] **Exit actor** — `AStreetExit` (C++ AActor + IInteractable): placed at each end of a street, reads its `ExitDirection` and target street from a property set in BP. If target is null or locked → collision blocks passage + no interaction prompt. If valid → prompt "Go [Direction]" → trigger street transition.
- [x] **Street transition** — `UStreetManager` (UGameInstanceSubsystem): `TransitionToStreet(UStreetDefinition*, EExitDirection)`. Saves player state, opens target level, repositions player at the matching AStreetExit spawn point via one-tick deferred HandleStreetArrival.
- [x] **Exit direction display** — `UStreetHUDWidget` (C++ UUserWidget): ArrowLeft/ArrowRight/ArrowUp Images shown/collapsed based on which AStreetExit actors have valid TargetStreet assigned.
- [ ] **World map widget (optional)** — `WBP_WorldMap`: top-down node graph of visited streets, current position highlighted, connections drawn as lines. Toggle with M key.

## NPC Interaction

NPCs with unique roles (drug dealer, father, young girl, etc.) that the player can talk to or give items to in order to get what they need.

- [ ] **NPC data asset** — `UNPCDefinition` (UDataAsset): NPC name, portrait texture, role tag, dialogue lines array (`TArray<FText>`), optional trade: `FNPCTradeOffer` (wants `UItemDefinition* RequiredItem + int32 Count`, gives `UItemDefinition* RewardItem + int32 Count`).
- [ ] **NPC actor** — `ANPCActor` (C++ AActor + IInteractable): `UNPCDefinition* NPCDef` (EditAnywhere). Instant interaction → opens dialogue widget. Sprite/mesh assigned in Blueprint child.
- [ ] **Dialogue widget** — `UDialogueWidget` (C++ UUserWidget): portrait on left, NPC name + current dialogue line on right, Next button cycles lines, Close button dismisses. If `NPCDef` has a trade offer, show a "Give Item" button (enabled only if player has the required item).
- [ ] **Give item flow** — "Give Item" checks `InventoryComponent->CountItemByID(RequiredItemID) >= Count`; if true, calls `RemoveItemByID` + `TryAddItem(RewardItem)` + advances to a "thank you" dialogue line.
- [ ] **NPC state persistence** — track per-NPC state (trade completed, dialogue stage) by NPC ID in `UTwoDSurvivalSaveGame`. Prevents re-trading after reload.
- [ ] **Blueprint steps** — Create `WBP_Dialogue` (portrait Image, name Text, dialogue Text, Next/Give/Close buttons). Assign `DialogueWidgetClass` on `BP_BaseCharacter`. Create NPC Blueprint children (e.g. `BP_NPC_DrugDealer`, `BP_NPC_Father`) with mesh + `NPCDef` assigned.

## House Customization

Player can place, move, and remove furniture/decorations inside their base.

- [ ] **Placeable item data** — extend `UItemDefinition` with `bIsPlaceable` bool + `TSubclassOf<APlaceableActor> PlaceableClass` (EditCondition gated).
- [ ] **Placeable actor base** — `APlaceableActor` (C++): static mesh, collision, a `PlacementID` (FGuid for save/load). Blueprint children for each furniture piece.
- [ ] **Placement mode** — dedicated input (e.g. RMB while holding a placeable item) enters placement mode: ghost mesh follows cursor clamped to a grid, green/red tint based on overlap. Confirm (LMB) spawns the actor; cancel (Esc) exits.
- [ ] **Move / remove** — interact with a placed actor to get a context menu: Move (re-enter placement mode with that actor) or Pick Up (destroys actor, returns item to inventory).
- [ ] **Room zones** — optional axis-aligned box volumes that define valid placement areas; placement outside a zone is rejected. Allows "only place furniture indoors".
- [ ] **Persistence** — save placed actor transforms + class references in `USaveGame`; respawn them on load.
