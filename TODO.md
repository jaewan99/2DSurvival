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
- [x] **Dynamic hotbar slots** — start with 0, `HotbarBonus` on `UItemDefinition` grants slots (belt/jacket/bag). `ExpandHotbar`/`ShrinkHotbar` called by `OnInventoryChanged` recalculation.
- [x] **Blueprint-customizable slot widget** — `UHotbarSlotWidget` base with `BindWidgetOptional` SlotIcon/SlotKeyLabel/ActiveHighlight; `OnSlotRefreshed` BlueprintImplementableEvent.
- [ ] **Blueprint steps — Hotbar widget setup:**
  1. Create WBP_HotbarSlot (parent: `UHotbarSlotWidget`). Design freely. Optionally add: Image "SlotIcon", TextBlock "SlotKeyLabel", any Widget "ActiveHighlight".
  2. Create WBP_HotbarWidget (parent: `UHotbarWidget`). Add HorizontalBox named "SlotContainer". Set SlotWidgetClass = WBP_HotbarSlot in class defaults.
  3. Assign WBP_HotbarWidget to HotbarWidgetClass on BP_BaseCharacter (already done if previously set).
  4. Create item data assets for equipment: DA_Belt (HotbarBonus=1), DA_Jacket (HotbarBonus=1), DA_Satchel (HotbarBonus=2), DA_Backpack (HotbarBonus=2, BonusSlots=6). Assign icons.
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
- [x] **Environmental hazard zones (C++)** — `AHazardZone` (C++ AActor): `UBoxComponent` trigger, configurable `EffectsToApply` (TArray<EStatusEffect>), `DirectDamagePerInterval` (raw body HP), `EffectInterval` (default 2 s), `SFX_Ambient` looping sound. On player enter: apply effects immediately + start interval timer + play ambient. On exit: clear timer + stop ambient (effects persist until cured). Blueprint children configure each hazard type via Details panel.
- [ ] **Blueprint steps — Hazard zones** — Create a Blueprint child per hazard type (parent: `AHazardZone`):
  1. **`BP_FireHazard`** — `EffectsToApply = [Bleeding]`, `DirectDamagePerInterval = 5`. Assign a fire/flame mesh to the `Mesh` component. Assign a crackling fire sound to `SFX_Ambient`.
  2. **`BP_ToxicHazard`** — `EffectsToApply = [Poisoned]`, `DirectDamagePerInterval = 0`. Assign a gas cloud/particle mesh. Assign a hissing/bubbling sound to `SFX_Ambient`.
  3. **`BP_ColdHazard`** — `EffectsToApply = [Wet]`, `DirectDamagePerInterval = 0`. Assign a frost/mist mesh. Assign a wind/chill sound to `SFX_Ambient`.
  4. Resize the **`HazardBox`** component in each Blueprint's viewport to match the hazard's footprint.
  5. Place instances in any sublevel. Tune `EffectInterval` (lower = more aggressive) per variant in class defaults.
- [ ] **Blueprint steps — Sound effects** — Assign Sound Wave / Sound Cue assets in the editor (all EditDefaultsOnly, no C++ changes needed):
  - **BP_BaseCharacter** Details → Sound: `SFX_Interact`, `SFX_InventoryOpen`, `SFX_InventoryClose`, `SFX_CraftOpen`, `SFX_CraftClose`, `SFX_DialogueOpen`, `SFX_DialogueClose`, `SFX_HealthHUDToggle`
  - In **WBP_InventoryWidget** (or BP_BaseCharacter's `ToggleInventory` override): call `PlayUISound(SFX_InventoryOpen)` on open, `PlayUISound(SFX_InventoryClose)` on close
  - **BP_DoorActor** Details → Sound: `SFX_Open`, `SFX_Close`, `SFX_Locked`
  - **BP_WorldItem** Details → Sound: `SFX_Pickup`
  - **WBP_CraftingWidget** Class Defaults → Sound: `SFX_CraftSuccess`, `SFX_CraftFail`
  - **WBP_DialogueWidget** Class Defaults → Sound: `SFX_NextLine`, `SFX_TradeComplete`

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
## Crafting

- [x] **Recipe data asset** — `UCraftingRecipe` (UDataAsset): `RecipeID`, `Ingredients` (`FIngredientEntry` array of ItemID + Count), `OutputItemID`, `OutputCount`.
- [x] **Crafting component** — `UCraftingComponent` (C++) on the player: auto-scans all recipes + item defs via AssetRegistry. `CanCraft`, `TryCraft`, `FindItemDef` (public).
- [x] **Crafting UI** — `UCraftingWidget` (C++ UUserWidget): left `ScrollBox` recipe list, right detail panel (output icon/name, ingredient rows with have/need color-coded, craft button, status text). Binds `OnInventoryChanged` to refresh craftability.
- [x] **Item upgrading** — `UCraftingRecipe` gains optional `InputItemID` (FName) + `IsUpgradeRecipe()` helper. `CanCraft` also requires 1× InputItemID in inventory. `TryCraft` removes 1× InputItemID before consuming ingredients. Crafting widget: upgrade recipes prefixed with "↑ " in the list; base item shown in cyan at the top of the ingredient panel with a separator line; status text says "Upgraded!" on success.
- [ ] **Blueprint steps — Upgrade recipes** — Create a data asset per upgrade:
  1. Right-click → Miscellaneous → Data Asset → **UCraftingRecipe**. Name it `DA_Recipe_Upgrade_<ItemName>` (e.g. `DA_Recipe_Upgrade_SteelSword`).
  2. Set **InputItemID** = the ItemID of the base item being consumed (e.g. `IronSword`).
  3. Set **Ingredients** = the additional materials required (e.g. SteelIngot×2).
  4. Set **OutputItemID** = the upgraded item's ItemID (e.g. `SteelSword`), **OutputCount** = 1.
  5. Optionally set **MinCraftingLevel** = 2 to gate behind Crafting Lv2.
  6. The recipe auto-appears in the crafting UI prefixed with "↑" — no extra wiring needed.
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
- [ ] **Blueprint steps — Map widget** — Create Widget Blueprint `WBP_MapWidget` (parent: `UMapWidget`):
  1. Root → Canvas Panel named **`MapCanvas`** (fill the widget area, 800×500 recommended)
  2. Add a **Button** named **`CloseButton`** anchored top-right
  3. Assign `MapWidgetClass = WBP_MapWidget` on **BP_BaseCharacter** Details panel → UI
  4. Press **M** in-game to test the map opens and closes
- [ ] **Blueprint steps — City data** — Wire up the city system in the editor:
  1. Right-click Content Browser → Miscellaneous → Data Asset → **UCityDefinition**. Create one per city (e.g. `DA_City_Riverside`, `DA_City_Downtown`). Set **CityID**, **CityName**, **MapColor** (unique hue per city).
  2. Open each **DA_Street_*** data asset. Set **OwnerCity** to the correct `DA_City_*`. Leave null for highway/wilderness streets.
  3. On highway connector streets (segments between cities) set **bIsHighway = true** and leave **OwnerCity** null.
  4. Optionally set **MapLabel** (short text) on any street whose name is too long for the map node.

## NPC Interaction

NPCs with unique roles (drug dealer, father, young girl, etc.) that the player can talk to or give items to in order to get what they need.

- [x] **NPC data asset** — `UNPCDefinition` (UDataAsset): NPCID, NPCName, Portrait, DialogueLines, bHasTradeOffer, FNPCTradeOffer (RequiredItem/Count + RewardItem/Count), PostTradeDialogue.
- [x] **NPC actor** — `ANPCActor` (C++ AActor + IInteractable): NPCDef, bTradeCompleted, OnTradeCompleted (BlueprintAssignable). Instant interaction → opens dialogue widget. OnBoxEndOverlap auto-closes.
- [x] **Dialogue widget** — `UDialogueWidget` (C++ UUserWidget): PortraitImage, NPCNameText, DialogueText, NextButton cycles lines, GiveItemButton (enabled when player has required items), CloseButton. Portrait set via FSlateBrush + SetResourceObject.
- [x] **Give item flow** — CountItemByID → RemoveItemByID → TryAddItem reward → NotifyTradeCompleted → switch to PostTradeDialogue lines.
- [x] **NPC state persistence** — `TSet<FName> CompletedNPCTrades` in `UTwoDSurvivalSaveGame`. Saved via GetAllActorsOfClass scan; restored at LoadGame.
- [ ] **Blueprint steps — Dialogue widget** — Create Widget Blueprint `WBP_DialogueWidget` (parent: `UDialogueWidget`):
  1. Add **Image** named **`PortraitImage`** (left side, e.g. 128×128)
  2. Add **TextBlock** named **`NPCNameText`** (NPC name header)
  3. Add **TextBlock** named **`DialogueText`** (body, multi-line, wrapping on)
  4. Add **Button** named **`NextButton`** with a child TextBlock labelled "Next ▶"
  5. Add **Button** named **`GiveItemButton`** with a child TextBlock labelled "Give Item"
  6. Add **TextBlock** named **`GiveItemLabel`** (shows "Item Name ×Count" — sits near GiveItemButton)
  7. Add **Button** named **`CloseButton`** (top-right ✕)
  8. Assign `DialogueWidgetClass = WBP_DialogueWidget` on **BP_BaseCharacter** Details panel → UI
- [ ] **Blueprint steps — NPC data assets** — For each NPC:
  1. Right-click → Miscellaneous → Data Asset → **UNPCDefinition**. Name it `DA_NPC_<Role>` (e.g. `DA_NPC_DrugDealer`).
  2. Set **NPCID** (stable FName, never rename), **NPCName**, **Portrait** (Texture2D).
  3. Fill **DialogueLines** array with the NPC's lines of dialogue.
  4. If this NPC trades: enable **bHasTradeOffer**, set **RequiredItem** + **RequiredCount**, set **RewardItem** + **RewardCount**, fill **PostTradeDialogue** lines (shown after trade).
- [ ] **Blueprint steps — NPC actors** — For each NPC variant:
  1. Create Blueprint child of **ANPCActor** (e.g. `BP_NPC_DrugDealer`).
  2. Add a **Static Mesh Component** for the NPC's appearance.
  3. Set **NPCDef** = the matching `DA_NPC_*` asset in Details panel.
  4. Place `BP_NPC_*` instances in level sublevels. The **InteractionBox** (already in C++) handles proximity; no extra setup needed.
  5. Optionally bind **OnTradeCompleted** in the level Blueprint to unlock doors, trigger events, etc.

## House Customization

Player can place and pick up furniture/decorations inside their base.

- [x] **Placeable item data** — `bIsPlaceable` bool + `TSubclassOf<APlaceableActor> PlaceableClass` on `UItemDefinition`. `Furniture` added to `EItemCategory` for context menu gating.
- [x] **Placeable actor base** — `APlaceableActor` (C++ AActor + IInteractable): `UStaticMeshComponent` + `UBoxComponent`, `FGuid PlacementID`, `SourceItemDef`. `SetGhostMode`/`SetGhostValid`/`FinalizePlace` lifecycle. Press E → pick up (returns item to inventory + destroys self).
- [x] **Placement mode** — `ABaseCharacter::PlaceItem(SlotIndex, Inv)` (BlueprintCallable) enters mode: spawns ghost actor, positions it via mouse deprojection onto Y=PlayerY plane each Tick, snaps to `PlacementGridSize` (default 50 cm) on X and Z. `OverlapAnyTestByChannel` drives green/red tint via `PlacementValidMaterial`/`PlacementInvalidMaterial`. LMB confirms (calls `ConfirmPlacement`), Escape cancels.
- [x] **Pick up** — Press E on a placed actor → `TryAddItem` → self-destroy (same pattern as `AWorldItem`). "Move" = pick up and immediately re-enter placement mode via context menu.
- [x] **Persistence** — `FPlacedActorSaveData` struct (PlacementID + FSoftClassPath + Transform + ItemDefID) in `UTwoDSurvivalSaveGame::PlacedActors`. SaveGame scans all `APlaceableActor`s (excluding ghosts); LoadGame destroys existing placed actors and respawns from save data.
- [ ] **Blueprint steps — Placeable items:**
  1. Create two simple translucent materials in the Content Browser:
     - **`M_PlacementValid`** — translucent, green tint (BaseColor = green, Opacity ≈ 0.5).
     - **`M_PlacementInvalid`** — translucent, red tint (BaseColor = red, Opacity ≈ 0.5).
     Assign both to **BP_BaseCharacter** Details → Placement → `PlacementValidMaterial` / `PlacementInvalidMaterial`.
  2. For each piece of furniture, create a Blueprint child of `APlaceableActor` (e.g. **`BP_Chair`**, **`BP_Table`**). In the Blueprint viewport, assign a Static Mesh to the **Mesh** component. Optionally resize **InteractionBox** to match the mesh footprint.
  3. Create an item data asset for each piece (e.g. **`DA_Chair`**): set `ItemCategory = Furniture`, `bIsPlaceable = true`, `PlaceableClass = BP_Chair`. Assign an icon.
  4. In **WBP_ContextMenu**: add a **Button** named **`Btn_Place`** (Is Focusable = false). In Event Construct, show it when `SlotData.ItemDef.bIsPlaceable == true`. OnClicked → call `PlaceItem(SlotIndex, OwnerComp)` on the player → Remove from Parent.
  5. Press **E** on a placed actor to pick it back up. To move: pick up, then place again.

## Status Effects

Tick-based debuffs applied to the player from wounds, environment, or enemy attacks. Very Project Zomboid.

- [x] **Status effect types** — `EStatusEffect` enum: Bleeding, Infected, Poisoned, BrokenBone, **Frostbite**, **Hypothermia**, **Wet**, **Concussion**. `FActiveStatusEffect` struct: Type + Severity + RemainingDuration (-1 = indefinite).
- [x] **`UStatusEffectComponent` (C++)** — on BaseCharacter. `TArray<FActiveStatusEffect>` ticked each frame. `ApplyEffect`, `RemoveEffect`, `HasEffect`, `GetSpeedMultiplier`, `GetDamageMultiplier`. `FOnStatusEffectsChanged` delegate.
- [x] **Per-effect behaviour:**
  - Bleeding 0.3 HP/s → Infected (after 120 s untreated) 0.1 HP/s
  - Poisoned 0.2 HP/s + 2× Hunger/Thirst drain
  - BrokenBone speed ×0.5 | Frostbite speed ×0.85, damage ×0.8 | Hypothermia speed ×0.7, 0.15 HP/s | Wet 1.5× Thirst drain | Concussion speed ×0.8, damage ×0.7
- [x] **Automatic progressions** — Wet + Snow outdoors 60 s → Frostbite. Frostbite + Snow outdoors 120 s → Hypothermia. Wet auto-applies while raining/snowing outdoors; fades 60 s after going indoors.
- [x] **Wound triggers** — `TakeMeleeDamage`: >25 dmg = 30% Bleeding; >40 dmg = 20% Concussion (45 s).
- [x] **Cure items** — `TArray<EStatusEffect> StatusEffectCures` on `UItemDefinition`. `UseItem_Implementation` iterates and removes. E.g. Bandage={Bleeding}, Antibiotics={Infected}, Antidote={Poisoned}, Splint={BrokenBone}, HotSoup={Frostbite,Hypothermia,Wet}, Painkillers={Concussion}.
- [x] **Status HUD** — `UStatusEffectWidget` (C++): `BindWidget EffectContainer` (VerticalBox). Shows red TextBlock per active effect, collapsed when empty. `OnEffectsRefreshed` BlueprintImplementableEvent.
- [x] **Save/load** — `TArray<FActiveStatusEffect> SavedStatusEffects` in SaveGame. Wet excluded (re-derived from weather on next tick).
- [ ] **Blueprint steps — Status Effects:**
  1. Create Widget Blueprint **`WBP_StatusEffects`** (parent: `UStatusEffectWidget`). Add VerticalBox named **`EffectContainer`**. Position it on-screen (e.g. top-left corner). Optionally override `OnEffectsRefreshed` to swap TextBlocks for custom icons.
  2. Assign **`StatusEffectWidgetClass = WBP_StatusEffects`** on **BP_BaseCharacter** Details panel → UI.
  3. Create cure item data assets: **`DA_Bandage`** (Consumable, `StatusEffectCures = [Bleeding]`), **`DA_Antibiotics`** (`[Infected]`), **`DA_Antidote`** (`[Poisoned]`), **`DA_Splint`** (`[BrokenBone]`), **`DA_HotSoup`** (`[Frostbite, Hypothermia, Wet]`), **`DA_Painkillers`** (`[Concussion]`). Assign icons.

## Flashlight

Equippable light source critical for nighttime exploration.

- [x] **Flashlight item** — `bIsFlashlight` + `FlashlightClass` on `UItemDefinition`. Equipping spawns `AFlashlightActor` attached to `FlashlightSocket`. Light on by default when equipped.
- [x] **AFlashlightActor (C++)** — `USpotLightComponent` (cone 25°, 1200 cm). `BatteryCharge` (0–100) drains at `DrainRate`/s while on. `Toggle()` / `RefillBattery(Amount)`. `IsInCone(WorldPos)` for enemy aggro query. Tune visuals in BP_FlashlightActor SpotLight component.
- [x] **Battery item** — `BatteryRestoreAmount` float on `UItemDefinition` (Consumable). `UseItem_Implementation` calls `EquippedFlashlight->RefillBattery(Amount)` if flashlight is equipped.
- [x] **Toggle** — selecting the same hotbar slot while flashlight is already equipped calls `Toggle()` instead of re-equipping. Dead battery blocks toggling on.
- [x] **Enemy interaction** — `AEnemyBase::DetectPlayer` doubles `AggroRange` when enemy is within the flashlight's `OuterConeAngle`. Uses `AFlashlightActor::IsInCone()`.
- [ ] **Blueprint steps — Flashlight:**
  1. Create socket **`FlashlightSocket`** on the character skeleton in the Skeleton editor (place near the hand/chest).
  2. Create Blueprint child **`BP_FlashlightActor`** (parent: `AFlashlightActor`). Tune the SpotLight component: Intensity, cone angles, attenuation radius, light color.
  3. Create item data asset **`DA_Flashlight`**: set `bIsFlashlight = true`, `FlashlightClass = BP_FlashlightActor`, assign an Icon.
  4. Create item data asset **`DA_Battery`**: set `ItemCategory = Consumable`, `BatteryRestoreAmount = 100`, assign an Icon.
  5. Set **`FlashlightSocketName = "FlashlightSocket"`** on **BP_BaseCharacter** Details → Flashlight.

## Sight Environment

Backside-darkening effect — the character's back face appears dim and desaturated. C++ already pushes `CharacterForward` each tick; only material editor work remains.

- [x] **C++ side** — `CharacterMIDs` created in `BeginPlay`; `CharacterForward` vector pushed via MID every `Tick`.
- [ ] **Blueprint steps — Sight Environment material** — Open the character's material in the Material Editor:
  1. Add a **Vector Parameter** node named **`CharacterForward`**, default value `(1, 0, 0, 0)`.
  2. Compute **BackFactor**: `VertexNormalWS` → `Dot Product` (with CharacterForward) → `Negate` → `Clamp(0, 1)`.
  3. Darken back: `Multiply(BaseColor, 0.15)` → `Lerp(A=BaseColor, B=darkened, Alpha=BackFactor)` → call this **DimColor**.
  4. Desaturate: `Multiply(BackFactor, 0.7)` → use as Fraction input of `Desaturation(DimColor)` → connect result to the **Base Color** pin.
  5. (Optional) Flatten normals on back: `Multiply(BackFactor, 0.85)` → `Lerp(A=NormalMap, B=Constant3(0,0,1), Alpha=result)` → connect to **Normal** pin.
  6. Save and compile the material. No Blueprint logic needed — C++ drives the parameter automatically.
  - See `Source/plan/sight-environment.md` for full node graph diagrams and tuning values.

## Weather System

Randomised weather states that layer on top of the day/night cycle, affecting visuals, needs, and enemy behaviour.

- [x] **AWeatherManager (C++ AActor)** — tick-driven state machine: `Clear`, `Cloudy`, `Rain`, `HeavyRain`, `Snow`. Random duration per state (EditAnywhere min/max). Season-weighted transitions (Spring: rain 20%, Summer: heavy rain 10%, Winter: snow 20%, no rain).
- [x] **Rain/Snow states** — pushes fog density offset + scene tint multiplier to `ATimeManager`'s shared components. Plays ambient sound loop. Thirst restore rate boosted (Rain +0.02/s, HeavyRain +0.04/s). Mood drained extra when outdoors (Rain 0.02/s, HeavyRain 0.04/s, Snow 0.01/s).
- [x] **`FOnWeatherChanged` delegate** — broadcast on every state transition. `bIsRaining` / `bIsSnowing` BlueprintReadOnly flags for other systems to query.
- [x] **Shelter detection** — `bIsIndoors` on `UNeedsComponent` set by `ABuildingEntrance` on enter/exit. Weather thirst/mood effects skip when indoors.
- [x] **Save/load** — `SavedWeatherState` + `SavedWeatherElapsed` in `UTwoDSurvivalSaveGame`. Restored via `AWeatherManager::RestoreWeather()` on load.
- [x] **Calendar system** — `ATimeManager` gains `CurrentDay/Month/Year`, `DaysPerMonth` (default 30), `ESeason` enum (Spring months 1–3, Summer 4–6, Fall 7–9, Winter 10–12). `OnDayChanged` delegate fires each midnight wrap. `GetDateText()` / `GetCurrentSeason()` / `SetCalendar()` public API. Calendar saved/loaded via `SavedDay/Month/Year/TimeOfDay`.
- [ ] **Blueprint steps — WeatherManager** — Set up the weather system in the editor:
  1. Create Blueprint child **`BP_WeatherManager`** (parent: `AWeatherManager`). Place one instance in the persistent level.
  2. In **BP_WeatherManager** Details panel → Weather|Sound: assign **`SFX_Rain`**, **`SFX_HeavyRain`**, **`SFX_Snow`** sound assets (Sound Wave or Sound Cue).
  3. Optionally tune **`MinStateDurationSeconds`** / **`MaxStateDurationSeconds`** (defaults: 120 s / 600 s).
  4. Press Play — weather will transition automatically. Check the Output Log for `[WeatherManager] State →` messages to confirm it's running.

## Journal System

A personal notebook that auto-records NPC encounters and exchanges. Press J to open.

- [x] **`FJournalEntry` struct** — NoteText, SourceNPCID, Day/Month/Year (stamped from ATimeManager).
- [x] **`UJournalComponent` (C++)** — on BaseCharacter. `AddEntry()`, `HasNoteForNPC()`, `AddNPCMetNote()`, `AddNPCTradeNote()`. `FOnJournalUpdated` delegate.
- [x] **Auto-notes from NPC interactions** — First meeting: "Met [Name]. They want Nx [Item] for Nx [Item]." Trade complete: "[Name]: Trade complete. Gave Nx [Item], received Nx [Item]." Logged via `ANPCActor::OnInteract` and `NotifyTradeCompleted`.
- [x] **`UJournalEntryWidget` (C++)** — BindWidgetOptional DateText/NoteText. `SetEntryData()`, `OnEntryRefreshed` BlueprintImplementableEvent.
- [x] **`UJournalWidget` (C++)** — ScrollBox EntryList, `EntryWidgetClass` (EditDefaultsOnly), `RebuildEntries()` bound to `OnJournalUpdated`. Newest first.
- [x] **J key toggle** — programmatic `IA_ToggleJournal` mapped to J. Shows/hides `UJournalWidget` instance. Follows same ShowUICursor/HideUICursor pattern.
- [x] **Save/load** — `TArray<FJournalEntry> JournalEntries` in `UTwoDSurvivalSaveGame`.
- [ ] **Blueprint steps — Journal widget:**
  1. Create Widget Blueprint **`WBP_JournalEntry`** (parent: `UJournalEntryWidget`). Add TextBlock **`DateText`** (small, grey) and TextBlock **`NoteText`** (main body, wrapping). Style freely. Override `OnEntryRefreshed` for custom per-note styling.
  2. Create Widget Blueprint **`WBP_JournalWidget`** (parent: `UJournalWidget`). Add ScrollBox named **`EntryList`** (fill the widget). Set **`EntryWidgetClass = WBP_JournalEntry`** in class defaults.
  3. Assign **`JournalWidgetClass = WBP_JournalWidget`** on **BP_BaseCharacter** Details panel → UI.
  4. Press **J** in-game after talking to an NPC — a note should appear automatically.

## Random World Events

Scheduled events that fire on day/night transitions, keeping the world feeling alive.

- [x] **FWorldEvent struct** — EventID (FName), `EWorldEventType` enum (TravellingTrader / ScavengerRaid / SupplyCrate), SpawnChance (float 0–1), MinDaysBetween (int32). Per-type config fields: TraderClass / EnemyClass+MinMax / CrateLootTable+Rolls.
- [x] **AWorldEventManager (C++ AActor)** — binds `ATimeManager::OnDayPhaseChanged`. On each night start: iterates `EventTable (TArray<FWorldEvent>)`, checks `MinDaysBetween` cooldown per EventID, rolls SpawnChance, fires the event. Dawn auto-despawns the trader.
- [x] **Travelling Trader** — spawns `TraderClass` (ANPCActor child) near the player on night start. Destroyed automatically at next dawn. Configure trade via NPCDef on the BP_NPC_Trader.
- [x] **Scavenger Raid** — spawns `MinEnemies–MaxEnemies` enemies of `EnemyClass` scattered near the player.
- [x] **Supply Crate** — spawns `MinCrateRolls–MaxCrateRolls` `AWorldItem` pickups from `CrateLootTable` (uses existing `FLootEntry` DropChance/MinCount/MaxCount).
- [x] **HUD notification** — `FOnWorldEventStarted` delegate on `AWorldEventManager`; `UStreetHUDWidget` binds it in `NativeConstruct` and shows `EventBanner` TextBlock for 5 s via `ShowNotification()`.
- [ ] **Blueprint steps — World Events:**
  1. Create Blueprint child **`BP_WorldEventManager`** (parent: `AWorldEventManager`). Place one instance in the persistent level.
  2. In Details → Events → **EventTable**, add entries (one per event variant):
     - All entries: set **EventID** (unique FName), **EventType**, **SpawnChance** (0–1), **MinDaysBetween**, **BannerText** (e.g. "A trader has arrived!").
     - **TravellingTrader** entry: set **TraderClass** = `BP_NPC_Trader` (an ANPCActor child with a trade NPCDef assigned in its Details panel).
     - **ScavengerRaid** entry: set **EnemyClass** = `BP_EnemyBase`, **MinEnemies** = 2, **MaxEnemies** = 4.
     - **SupplyCrate** entry: fill **CrateLootTable** (ItemDef + DropChance + MinCount/MaxCount per row), set **MinCrateRolls** = 3, **MaxCrateRolls** = 5.
  3. Open **WBP_StreetHUD**. Add a **TextBlock** named exactly **`EventBanner`** (top-center, large font). Set its default **Visibility = Collapsed**. The C++ shows and hides it automatically.
  4. Press **Play**, advance time to night (or set `DayDurationSeconds = 30` temporarily), and check the Output Log for `[WorldEventManager]` messages confirming events fire.

## Skill / XP System

Passive progression that rewards consistent playstyle choices.

- [ ] **ESkillType enum** — `Combat`, `Crafting`, `Scavenging`.
- [ ] **USkillComponent (C++)** — on BaseCharacter. `TMap<ESkillType, int32> XP` + `TMap<ESkillType, int32> Level`. `AddXP(ESkillType, Amount)`: accumulates XP, levels up at thresholds (100 × Level), broadcasts `FOnSkillLevelUp`. `GetLevel(ESkillType)` queried by other systems.
- [ ] **XP sources** — Combat: `+15 XP` per enemy kill (bound to `AEnemyBase::OnDeath` via delegate). Crafting: `+10 XP` per successful craft (bound to `UCraftingComponent::OnCraftingChanged`). Scavenging: `+5 XP` per item pickup (in `AWorldItem::OnInteract`).
- [ ] **Level bonuses** — Combat Lv2: +10% melee damage; Lv3: attack cooldown –15%. Crafting Lv2: unlocks a second tier of recipes (check `GetLevel(Crafting) >= 2` in `CanCraft`); Lv3: 10% chance to craft double output. Scavenging Lv2: +1 extra item roll on enemy loot tables; Lv3: `AWorldItem` pickup range +50%.
- [x] **Skill HUD (C++)** — `USkillRowWidget` (SkillLabel, LevelText, XPBar, optional XPText; per-skill bar colors; `OnLevelUp` BlueprintImplementableEvent). `USkillHUDWidget` (3 rows + optional TitleBar drag handle; binds `OnSkillXPChanged`/`OnSkillLevelUp` delegates; refreshes only the changed row). Toggle with **K** key (programmatic `IA_ToggleSkillHUD`). Spawns at (50, 220) below the Health HUD default position.
- [ ] **Blueprint steps — Skill HUD:**
  1. Create Widget Blueprint **`WBP_SkillRow`** (parent: `USkillRowWidget`). Add: TextBlock **`SkillLabel`**, TextBlock **`LevelText`**, ProgressBar **`XPBar`**. Optionally add TextBlock **`XPText`** for "75 / 100" display. Style freely. Optionally override **`OnLevelUp`** to play a flash animation.
  2. Create Widget Blueprint **`WBP_SkillHUD`** (parent: `USkillHUDWidget`). Add three **`WBP_SkillRow`** instances named exactly **`Row_Combat`**, **`Row_Crafting`**, **`Row_Scavenging`**. Optionally add a Border named **`TitleBar`** at the top for drag-to-move.
  3. Assign **`SkillHUDWidgetClass = WBP_SkillHUD`** on **BP_BaseCharacter** Details panel → UI.
  4. Press **K** in-game to open/close the panel. Kill an enemy (+15 Combat XP), craft (+10 Crafting XP), or pick up an item (+5 Scavenging XP) to see bars fill.
- [ ] **Save/load** — serialize `XP` + `Level` maps into `UTwoDSurvivalSaveGame`.

## Low Priority

- [ ] **Enemy spawner** — Blueprint Actor with spawn radius, max count, respawn delay; spawns `BP_EnemyBase` variants in the world.
