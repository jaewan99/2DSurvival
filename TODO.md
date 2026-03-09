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

- [x] **Door** — `ADoorActor` (C++ AActor + IInteractable). `DoorMesh` (BlockAll closed / NoCollision open), `InteractionBox`, `bIsLocked`, `bStartOpen`. Prompt reads "Open Door" / "Close Door" / "Locked". Blueprint child `BP_DoorActor` (assign mesh).

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

- [x] **Recipe data asset** — `UCraftingRecipe` (UDataAsset): `RecipeID`, `Ingredients` (`FIngredientEntry` array of ItemID + Count), `OutputItemID`, `OutputCount`.
- [x] **Crafting component** — `UCraftingComponent` (C++) on the player: auto-scans all recipes + item defs via AssetRegistry. `CanCraft`, `TryCraft`, `FindItemDef` (public).
- [x] **Crafting UI** — `UCraftingWidget` (C++ UUserWidget): left `ScrollBox` recipe list, right detail panel (output icon/name, ingredient rows with have/need color-coded, craft button, status text). Binds `OnInventoryChanged` to refresh craftability.
- [ ] **Item upgrading** — extend `UCraftingRecipe` with an optional `InputItemDef` (item being upgraded) + `OutputItemDef` (upgraded result). `UCraftingComponent->Upgrade(Recipe, SlotIndex)` removes the base item and adds the upgraded one.
- [x] **Crafting station** — `ACraftingTable` (C++ AActor + IInteractable): `UBoxComponent InteractionBox`, press E → `ToggleCrafting()`. `OnBoxEndOverlap` calls `Player->CloseCrafting()` when player walks away.

## Moods / Needs

Inspired by Project Zomboid — needs decay over time and affect gameplay stats.

- [x] **Needs component** — `UNeedsComponent` (C++) on the player. Tracks Hunger/Thirst/Fatigue (0–100). Tick-driven drain; doubled when running/attacking. `FOnNeedChanged` delegate.
- [x] **Need types** — Hunger (0.055/s), Thirst (0.075/s), Fatigue (0.04/s). At 0: 0.5 HP/s DoT on Body. Below 50%: speed ×0.7, damage ×0.8.
- [x] **Need effects** — speed/damage multipliers applied via `RecalculateMovementSpeed()` and `OnHitboxOverlap`. Critical HP drain via `HealthComponent->ApplyDamage`.
- [x] **Satisfying needs** — `HungerRestore`/`ThirstRestore`/`FatigueRestore` on `UItemDefinition`. Sleeping (ABedActor) restores Fatigue at 3× rate; auto-wakes when full (if started below 95%).
- [x] **Needs HUD** — `UNeedsWarningWidget` (C++): `NeedsContainer` VerticalBox BindWidget, 3 icon slots built dynamically. Icons collapse when need ≥ 50%, appear when critical. Blueprint child `WBP_NeedsWarning`.
- [x] **Mood system** — `UNeedsComponent` gains `Mood` float (0–100), drain/boost events, `FOnMoodChanged` delegate. `ABaseCharacter` creates `UPostProcessComponent (bUnbound=true)` and lerps desaturation/cool tint at low mood. Saved/loaded via `SavedMood` in `UTwoDSurvivalSaveGame`.

## World Map

Each "map" is a street or building sublevel streamed seamlessly. Exits are now named (`FName ExitID`) — no fixed Left/Right/Up limit.

- [x] **Street data asset** — `UStreetDefinition` (UDataAsset): `StreetID`, `StreetName`, `StreetWidth`, `TSoftObjectPtr<UWorld> Level`, `TArray<FStreetExitLink> Exits` (each: `ExitID` FName + `Destination` + `EExitLayout`), `bIsPCGBuilding`. `GetExit(FName)` helper.
- [x] **Exit actor** — `AStreetExit` (C++ AActor, walk-through trigger): `FName ExitID` matches a `FStreetExitLink` in the current street's definition. Player walks through → `SM->OnPlayerCrossedExit(ExitID)`. Logs warning if ExitID is None.
- [x] **Exit spawn point** — `AExitSpawnPoint` (C++ AActor): `FName SpawnID` placed in sublevels. Manager teleports player here on Building-layout arrivals (SpawnID matches incoming ExitID). Editor-only blue `UArrowComponent`. Fallback to `ABuildingEntrance` if not found.
- [x] **Street transition** — `UStreetManager`: `OnPlayerCrossedExit(FName)` looks up exit link, loads adjacent (AdjacentRight/Left) or building (Building) level. `OnPlayerExitBuilding()` restores return street. `bTransitionInProgress` guard. `FOnStreetChanged` delegate.
- [x] **Building entrance** — `ABuildingEntrance` (C++ + IInteractable): `FName BuildingExitID` (street-side) — calls `SM->OnPlayerCrossedExit(BuildingExitID)`. Building-side calls `SM->OnPlayerExitBuilding()`. Prompt reads `bIsInsideBuilding`.
- [x] **Exit direction display** — `UStreetHUDWidget`: arrows shown for exits named `"Left"`, `"Right"`, `"Up"` by convention. Name exits this way to drive the default HUD arrows.
- [x] **World map widget** — `UMapWidget` (C++ UUserWidget): BFS node graph from starting street, city-colored region backgrounds, grey street lines, amber highway lines drawn via `NativePaint`. Visited streets tracked in `UStreetManager::VisitedStreetIDs`. Toggle with M key. Blueprint child `WBP_MapWidget` (CanvasPanel "MapCanvas" + Button "CloseButton").
- [x] **City system** — `UCityDefinition` (UDataAsset): CityID, CityName, MapColor. `UStreetDefinition` extended with `OwnerCity`, `bIsHighway`, `MapLabel`. `UStreetManager` tracks `CurrentCity`, `VisitedStreetIDs`, `StartingStreetDef`; broadcasts `OnCityChanged` on city transition. Map widget groups streets into colored city regions. Blueprint setup: create `DA_City_*` assets, set `OwnerCity` on each `DA_Street_*`, set `bIsHighway=true` on highway streets.
- [ ] **Blueprint steps — Map** — Create `WBP_MapWidget` (CanvasPanel "MapCanvas" + Button "CloseButton"). Assign `MapWidgetClass` on `BP_BaseCharacter`.

## NPC Interaction

NPCs with unique roles (drug dealer, father, young girl, etc.) that the player can talk to or give items to in order to get what they need.

- [x] **NPC data asset** — `UNPCDefinition` (UDataAsset): NPCID, NPCName, Portrait, DialogueLines, bHasTradeOffer, FNPCTradeOffer (RequiredItem/Count + RewardItem/Count), PostTradeDialogue.
- [x] **NPC actor** — `ANPCActor` (C++ AActor + IInteractable): NPCDef, bTradeCompleted, OnTradeCompleted (BlueprintAssignable). Instant interaction → opens dialogue widget. OnBoxEndOverlap auto-closes.
- [x] **Dialogue widget** — `UDialogueWidget` (C++ UUserWidget): PortraitImage, NPCNameText, DialogueText, NextButton cycles lines, GiveItemButton (enabled when player has required items), CloseButton. Portrait set via FSlateBrush + SetResourceObject.
- [x] **Give item flow** — CountItemByID → RemoveItemByID → TryAddItem reward → NotifyTradeCompleted → switch to PostTradeDialogue lines.
- [x] **NPC state persistence** — `TSet<FName> CompletedNPCTrades` in `UTwoDSurvivalSaveGame`. Saved via GetAllActorsOfClass scan; restored at LoadGame.
- [ ] **Blueprint steps** — Create `WBP_DialogueWidget` (PortraitImage, NPCNameText, DialogueText, NextButton, GiveItemButton, GiveItemLabel, CloseButton). Assign `DialogueWidgetClass` on `BP_BaseCharacter`. Create `DA_NPC_*` data assets. Create `BP_NPCActor` children with mesh + NPCDef assigned.

## House Customization

Player can place, move, and remove furniture/decorations inside their base.

- [ ] **Placeable item data** — extend `UItemDefinition` with `bIsPlaceable` bool + `TSubclassOf<APlaceableActor> PlaceableClass` (EditCondition gated).
- [ ] **Placeable actor base** — `APlaceableActor` (C++): static mesh, collision, a `PlacementID` (FGuid for save/load). Blueprint children for each furniture piece.
- [ ] **Placement mode** — dedicated input (e.g. RMB while holding a placeable item) enters placement mode: ghost mesh follows cursor clamped to a grid, green/red tint based on overlap. Confirm (LMB) spawns the actor; cancel (Esc) exits.
- [ ] **Move / remove** — interact with a placed actor to get a context menu: Move (re-enter placement mode with that actor) or Pick Up (destroys actor, returns item to inventory).
- [ ] **Room zones** — optional axis-aligned box volumes that define valid placement areas; placement outside a zone is rejected. Allows "only place furniture indoors".
- [ ] **Persistence** — save placed actor transforms + class references in `USaveGame`; respawn them on load.
