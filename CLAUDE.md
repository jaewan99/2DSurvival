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

## C++ First Philosophy

**All logic goes in C++. Blueprint is only for:**
1. Visual layout of UMG widgets (drag widgets onto canvas, set widget names to match BindWidget)
2. Assigning asset references in the Details panel (textures, sounds, widget classes, input actions)
3. Creating socket names in the Skeleton editor

**Never implement logic in Blueprint.** If something seems to require Blueprint logic, find the C++ way:
- Delegate bindings → `AddDynamic` in C++
- Widget construction → `NativeConstruct` override
- Input handling → `SetupPlayerInputComponent` or `CreateInputActions` in C++
- UI refresh → centralized `RefreshAll()` called from C++ delegate
- Programmatic input → `NewObject<UInputAction>()` + `NewObject<UInputMappingContext>()` in `BeginPlay`
- Asset scanning → `FAssetRegistryModule` (no manual `AllItemDefinitions` array to fill)
- Widget creation → `CreateWidget<T>()` + `AddToViewport()` in C++ BeginPlay or toggle functions

**Accepted Blueprint steps per feature: 0–3 max** (widget layout + class assignment only).

## C++ / Blueprint Conventions

- **BlueprintImplementableEvent** — can be overridden in Blueprint and called from C++, but is NOT reliably callable from OTHER Blueprints (e.g. a cabinet actor calling a function on the character).
- **BlueprintNativeEvent + BlueprintCallable** — use this whenever a function needs to be BOTH callable from Blueprint (including external Blueprints) AND overridable in Blueprint. Always declare the `virtual void FunctionName_Implementation()` in the header with an empty body.
- **BindWidget** — use for required widget references; **BindWidgetOptional** for optional ones. The Blueprint child just needs a widget with the matching name.
- **NativeConstruct** — the C++ equivalent of Event Construct. Bind delegates, cache component refs, and call initial refresh here.
- **WidgetTree->ConstructWidget<T>()** — use to create child widgets dynamically in C++ instead of Blueprint.

## Input System Pattern

All input is created programmatically in C++ — no Input Action assets need to be created in the editor for gameplay keys:

```cpp
// In CreateInputActions() called from BeginPlay:
UInputAction* IA_Jump = NewObject<UInputAction>(this, TEXT("IA_Jump"));
IA_Jump->ValueType = EInputActionValueType::Boolean;
GameplayIMC = NewObject<UInputMappingContext>(this, TEXT("GameplayIMC"));
GameplayIMC->MapKey(IA_Jump, EKeys::SpaceBar);

// In SetupPlayerInputComponent:
EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ABaseCharacter::OnJump);
```

Exception: `IA_Interact`, `IA_ToggleInventory`, `IA_ToggleHealthUI` remain as editor-assigned UPROPERTYs because they already exist as assets. New features should use programmatic creation.

## Asset Scanning Pattern

Never use a manual `AllItemDefinitions` array. Use AssetRegistry to auto-scan:

```cpp
FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
TArray<FAssetData> Assets;
ARM.Get().GetAssetsByClass(UItemDefinition::StaticClass()->GetClassPathName(), Assets);
for (const FAssetData& AD : Assets)
{
    UItemDefinition* Def = Cast<UItemDefinition>(AD.GetAsset());
    if (Def && !Def->ItemID.IsNone()) ItemDefMap.Add(Def->ItemID, Def);
}
```

Requires `"AssetRegistry"` in `PublicDependencyModuleNames` in `Build.cs`.

## UI Widget Pattern

All widgets are C++ `UUserWidget` subclasses with Blueprint children for layout only:

- `NativeConstruct` → get player pawn, find components, bind delegates, call initial refresh
- `RefreshAll()` / `RefreshSlots()` → single centralized refresh function, bound to component delegates
- Dynamic slot UIs → build with `WidgetTree->ConstructWidget<UBorder>()` etc. in `NativeConstruct`
- Widget creation from character → `CreateWidget<T>(PC, WidgetClass)` + `AddToViewport()` in C++ BeginPlay or toggle functions
- Toggle pattern → check if instance exists: destroy if yes, create if no

**Blueprint child only needs:** widget names matching `BindWidget` properties. Zero Blueprint logic.

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

## UI Refresh Pattern

- **Centralize refresh logic** — create ONE `RefreshAll()` function that updates all UI elements
- **Use delegate bindings** — bind `OnInventoryChanged` / `OnHotbarChanged` / `OnBodyPartDamaged` directly to refresh functions via `AddDynamic` in `NativeConstruct`
- **Avoid scattered manual refresh calls** — let delegate bindings handle updates automatically
- This prevents stale UI bugs and makes the system easy to reason about

## Movement / Physics Notes

- **DO NOT use `DisableMovement()`** to lock player input — it disables gravity, character hangs mid-air
- Use `bMovementLocked` bool on `ABaseCharacter` instead; `MoveRight` checks it and returns early
- Movement constrained to X/Z plane via `bConstrainToPlane` + `SetPlaneConstraintNormal(FVector(0,1,0))`
