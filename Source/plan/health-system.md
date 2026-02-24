# Health System — Implementation Reference

## Overview

Per-body-part health system replacing the previous single `Health` float on `ABaseCharacter`.
Each body part has its own health pool. Damage to different parts causes different mechanical effects.
Head or Body reaching 0 causes death.

**Implemented:** 2026-02-24

---

## Body Parts & Effects

| Part | Death on 0? | Mechanical Effect |
|---|---|---|
| Head | Yes | Instant death — `OnDeath` fires |
| Body | Yes | Instant death — `OnDeath` fires |
| Left Arm | No | Reduces weapon damage (`GetDamageMultiplier()`) |
| Right Arm | No | Reduces weapon damage (`GetDamageMultiplier()`) |
| Left Leg | No | Reduces movement speed (`GetMovementSpeedMultiplier()`) |
| Right Leg | No | Reduces movement speed (`GetMovementSpeedMultiplier()`) |

---

## Files

### New Files

| File | Purpose |
|---|---|
| `Public/Character/HealthTypes.h` | `EBodyPart` enum + `FBodyPartHealth` struct |
| `Public/Character/HealthComponent.h` | `UHealthComponent` declaration — delegates, config, API |
| `Private/Character/HealthComponent.cpp` | `UHealthComponent` implementation |

### Modified Files

| File | What Changed |
|---|---|
| `Public/Character/BaseCharacter.h` | Removed `Health`/`MaxHealth`; added `HealthComponent`, `BaseWalkSpeed`, private `OnBodyPartDamaged` handler |
| `Private/Character/BaseCharacter.cpp` | Constructor creates component; `BeginPlay` stores base speed + binds delegate; `UseItem` restores Body health; new handler adjusts speed / fires death |

---

## Architecture

### `HealthTypes.h`

```
EBodyPart  — Head | Body | LeftArm | RightArm | LeftLeg | RightLeg

FBodyPartHealth
  float MaxHealth     = 100.f
  float CurrentHealth = 100.f
  bool IsBroken()     → CurrentHealth <= 0
```

### `UHealthComponent`

**Delegates (BlueprintAssignable):**
```
FOnBodyPartDamaged  → (EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken)
FOnDeath            → ()
```

**Configurable max health (EditDefaultsOnly, set in BP_BaseCharacter defaults):**
```
HeadMaxHealth  = 50.f    ← fragile by design
BodyMaxHealth  = 100.f
ArmMaxHealth   = 75.f    ← shared for both arms
LegMaxHealth   = 75.f    ← shared for both legs
```

**Runtime state:**
```
TMap<EBodyPart, FBodyPartHealth> BodyParts   ← initialized in BeginPlay
```

**BeginPlay** — populates `BodyParts` from the per-part max health properties.

**Core API:**
```
ApplyDamage(Part, Amount)           → clamps to 0, fires OnBodyPartDamaged (bJustBroken = true on first break)
RestoreHealth(Part, Amount)         → clamps to MaxHealth
GetHealthPercent(Part) → float      → CurrentHealth / MaxHealth (0–1)
GetBodyPart(Part) → FBodyPartHealth → copy of the struct
IsDead() → bool                     → Head.IsBroken() || Body.IsBroken()
GetDamageMultiplier() → float       → (LeftArm% + RightArm%) / 2
GetMovementSpeedMultiplier() → float
```

**Movement speed multiplier table:**
```
Both legs healthy    → 1.00
One leg broken       → 0.75
Both legs broken     → 0.25
```

**Damage multiplier** — average of arm health percentages, applied externally to weapon `BaseDamage` when attacking.

> `OnDeath` is NOT broadcast inside `ApplyDamage`. It is broadcast by `BaseCharacter::OnBodyPartDamaged` when `bJustBroken == true` for Head or Body. This keeps the component reusable (can be placed on non-character actors) while centralizing character-specific death logic in the character.

---

### `ABaseCharacter` Changes

**Removed:**
- `float MaxHealth`
- `float Health`

**Added:**
```cpp
UHealthComponent* HealthComponent   // VisibleAnywhere — created in constructor
float BaseWalkSpeed                 // stored in BeginPlay from MaxWalkSpeed
```

**BeginPlay:**
```cpp
BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
HealthComponent->OnBodyPartDamaged.AddDynamic(this, &ABaseCharacter::OnBodyPartDamaged);
```

**`OnBodyPartDamaged` handler (private):**
```
LeftLeg or RightLeg → MaxWalkSpeed = BaseWalkSpeed * GetMovementSpeedMultiplier()
Head or Body broken → HealthComponent->OnDeath.Broadcast()
```

**`UseItem_Implementation`:**
- Calls `HealthComponent->RestoreHealth(EBodyPart::Body, HealthRestoreAmount)`
- Logs `Body Health: X / Y`

---

## Blueprint Usage

After compiling, in `BP_BaseCharacter` or any Blueprint:

| Function | Where to call |
|---|---|
| `HealthComponent → ApplyDamage(Part, Amount)` | Enemy attack, weapon hit, hazard |
| `HealthComponent → GetHealthPercent(Part)` | Bind to UI health bars per part |
| `HealthComponent → OnBodyPartDamaged` | Bind in BP for damage indicators / animations |
| `HealthComponent → OnDeath` | Bind in BP for death animation / respawn logic |
| `HealthComponent → GetDamageMultiplier()` | Multiply against `WeaponBase.BaseDamage` before dealing damage |

---

## Verification Checklist

- [ ] Compile — no errors
- [ ] Use `DA_Berry` (Consumable) → Body health restores
- [ ] `ApplyDamage(LeftLeg, 80)` → `MaxWalkSpeed` drops to 75% of base
- [ ] `ApplyDamage(RightLeg, 80)` (after above) → drops to 25% of base
- [ ] `ApplyDamage(Head, 999)` → `IsDead()` returns true, `OnDeath` fires once
- [ ] `ApplyDamage(LeftArm, 80)` → `GetDamageMultiplier()` returns ~0.6 (60/75 arm + 100/75 arm clamped — actually 0.4 + 1.0 / 2 = 0.7)

---

## Known Design Decisions

- **`OnDeath` source of truth** — fired by `BaseCharacter::OnBodyPartDamaged`, not inside `HealthComponent.ApplyDamage`. Avoids double-broadcast if the character class is extended, and keeps the component reusable on non-character actors.
- **`BaseWalkSpeed` stored at BeginPlay** — not hardcoded, so changing `MaxWalkSpeed` in BP defaults automatically works.
- **`ArmMaxHealth` shared for both arms** — simplicity; can be split into `LeftArmMaxHealth`/`RightArmMaxHealth` later if asymmetric damage is needed.
- **Consumable restores `EBodyPart::Body`** — food/healing is most naturally "core body" recovery. Future items could restore specific parts.

---

## Health HUD (WBP_HealthHUD) — Full Implementation Guide

**Added:** 2026-02-24
**C++ changes already compiled.** This section covers everything still needed in the Unreal Editor.

---

### C++ Already Done

| What | Status |
|---|---|
| `IA_ToggleHealthUI` UPROPERTY on `ABaseCharacter` | ✅ Done |
| `ToggleHealthUI` BlueprintNativeEvent declaration | ✅ Done |
| `SetupPlayerInputComponent` binds `IA_ToggleHealthUI → ToggleHealthUI` | ✅ Done |
| Compile | ✅ Clean |

---

### Step 1 — Create Input Action Asset

1. Content Browser → right-click → **Input → Input Action**
2. Name: `IA_ToggleHealthUI`
3. Open it → **Value Type: Digital (bool)** — no triggers
4. Save

---

### Step 2 — Add H Key to IMC

1. Open your **Input Mapping Context** (e.g. `IMC_Default`)
2. Click **+** → select `IA_ToggleHealthUI`
3. Key: **H**
4. Save

---

### Step 3 — Create WBP_BodyPartRow (reusable sub-widget)

1. Content Browser → right-click → **User Interface → Widget Blueprint** → name `WBP_BodyPartRow`
2. Open → **Designer**

**Hierarchy:**
```
Horizontal Box  (root)
  ├─ Text  "PartLabel"    Size Box: width 70px, Right-align text, Fill vertical
  ├─ Progress Bar "HealthBar"   Fill horizontal
  └─ Text  "BrokenText"   content "BROKEN", Visibility: Collapsed
```

**Create Function `SetData`** — inputs: `PartName` (Name), `Percent` (Float):

```
PartLabel  → Set Text (PartName → ToString)
HealthBar  → Set Percent (Percent)

Color logic (use Select or Branch chain):
  Percent > 0.6               → Green  (0.1, 0.8, 0.1, 1.0)
  Percent >= 0.3 AND <= 0.6   → Yellow (0.9, 0.8, 0.1, 1.0)
  Percent > 0.0 AND < 0.3     → Red    (0.9, 0.2, 0.1, 1.0)
  Percent <= 0.0              → Grey   (0.4, 0.4, 0.4, 1.0)

HealthBar → Set Fill Color and Opacity (color from above)

BrokenText → Set Visibility:
  Percent <= 0.0 → Visible
  else           → Collapsed
```

4. Save

---

### Step 4 — Create WBP_HealthHUD (main panel)

1. Content Browser → right-click → **User Interface → Widget Blueprint** → name `WBP_HealthHUD`
2. Open → **Class Settings** → **uncheck Is Focusable** (critical — must not steal H key focus)

**Hierarchy:**
```
Canvas Panel  (root)
  └─ Border "Panel"
       Size: ~260 × 280, Anchors: top-left, Position: 0,0
       Brush Color: (0.05, 0.05, 0.05, 0.85)
       └─ Vertical Box
            ├─ Border "TitleBar"
            │    Height: 28px, Brush Color: (0.12, 0.12, 0.12, 1.0), Padding: 8,4
            │    └─ Text  "♥  Health"   Font size 13, Bold
            └─ Vertical Box "BodyRows"  Padding: 8px all sides
                 ├─ WBP_BodyPartRow "Row_Head"
                 ├─ WBP_BodyPartRow "Row_Body"
                 ├─ Border (separator — height 1px, Brush Color white alpha 0.15)
                 ├─ WBP_BodyPartRow "Row_LeftArm"
                 ├─ WBP_BodyPartRow "Row_RightArm"
                 ├─ Border (separator)
                 ├─ WBP_BodyPartRow "Row_LeftLeg"
                 └─ WBP_BodyPartRow "Row_RightLeg"
```

**Add Variables (Graph panel):**

| Name | Type |
|---|---|
| `bIsDragging` | Boolean |
| `DragOffset` | Vector 2D |
| `CachedHealthComp` | Object Reference → `Health Component` |

---

**`RefreshAll` Function:**
```
Row_Head     → SetData("HEAD",   CachedHealthComp → GetHealthPercent(Head))
Row_Body     → SetData("BODY",   CachedHealthComp → GetHealthPercent(Body))
Row_LeftArm  → SetData("L.ARM",  CachedHealthComp → GetHealthPercent(LeftArm))
Row_RightArm → SetData("R.ARM",  CachedHealthComp → GetHealthPercent(RightArm))
Row_LeftLeg  → SetData("L.LEG",  CachedHealthComp → GetHealthPercent(LeftLeg))
Row_RightLeg → SetData("R.LEG",  CachedHealthComp → GetHealthPercent(RightLeg))
```

---

**Event Construct:**
```
1. Get Player Character → Cast to BP_BaseCharacter
2. CachedHealthComp = As BP_BaseCharacter → Health Component
3. CachedHealthComp → OnBodyPartDamaged → Bind Event → Custom Event → call RefreshAll
4. Call RefreshAll   (sets initial bar state)
5. Set Position in Viewport (X=20, Y=580, Remove DPI Scale=false)
```

---

**Dragging — TitleBar `On Mouse Button Down`:**
```
1. Get Mouse Position on Viewport → MousePos
2. Panel → GetCachedGeometry → GetAbsolutePosition → WidgetPos
3. DragOffset = MousePos - WidgetPos
4. bIsDragging = true
5. Return Handled
```

**Panel `On Mouse Move`:**
```
Branch: bIsDragging?
  True → Get Mouse Position on Viewport
         Set Position in Viewport (Pos - DragOffset, Remove DPI Scale = false)
         Return Handled
```

**Panel `On Mouse Button Up`:**
```
bIsDragging = false
Return Handled
```

4. Compile + Save

---

### Step 5 — Update BP_BaseCharacter

1. **Details panel** → Input category → assign `IA_ToggleHealthUI` to the new slot
2. **Add Variable:** `HealthHUDWidget` — type `WBP_HealthHUD` (object reference)
3. **Override `ToggleHealthUI`** (right-click in Event Graph → search ToggleHealthUI):

```
Branch: Is Valid(HealthHUDWidget)?

  True branch:
    HealthHUDWidget → Remove from Parent
    Set HealthHUDWidget = null

  False branch:
    Create Widget (Class = WBP_HealthHUD) → Set HealthHUDWidget
    HealthHUDWidget → Add to Viewport (Z-Order = 0)
```

4. Compile + Save

---

### Verification Checklist

- [ ] Compile clean ✅ (already done)
- [ ] Press **H** in PIE → HUD appears bottom-left, all bars full green
- [ ] Press **H** again → HUD disappears
- [ ] `ApplyDamage(Head, 30)` → Head bar shifts yellow/red
- [ ] `ApplyDamage(LeftLeg, 75)` → Left Leg bar reaches 0, shows **BROKEN** in grey
- [ ] Drag TitleBar → panel repositions freely
- [ ] Use a consumable → Body bar increases
