# Animation System

## Architecture

The AnimBP (`ABP_Player`) is the **built-in UE5 Blueprint-based Animation Blueprint**
created via the Unreal Engine "Animation Blueprint" template. It contains Blueprint
state machines, blend spaces, and IK rig nodes that cannot be migrated to C++.

### ⚠️ DO NOT create a custom C++ AnimInstance

Never subclass `UAnimInstance` in C++ for this project. Specifically:
- Do NOT create a `PlayerAnimInstance.h / .cpp` (was attempted and deleted)
- Do NOT reparent `ABP_Player` to a custom C++ class
- The existing AnimBP has Blueprint logic that will break if the parent class changes

All animation *variables* are driven from C++ via `ABaseCharacter` properties marked
`BlueprintReadOnly`. The AnimBP reads them in `Event Blueprint Update Animation`.
This is the correct pattern — C++ owns the data, Blueprint reads and visualises it.

---

## Variables the AnimBP reads from the character

In `Event Blueprint Update Animation`:
1. `Try Get Pawn Owner` → `Cast to BP_BaseCharacter` → store as `CharacterRef`
2. Read each property from `CharacterRef` and store as a local AnimBP variable

| Property on `ABaseCharacter` | Type | Purpose in AnimGraph |
|---|---|---|
| `bIsAttacking` | bool | Disable IK rig while any attack montage plays |
| `bMovementLocked` | bool | Optional: freeze locomotion during hold interactions |
| `MovementSpeedMultiplier` *(TODO)* | float | Scale blend space play rate when needs are low — **not yet added to C++, deferred to animation pass** |

---

## ⏳ Deferred: Needs Speed Penalty — Animation Hookup

`MaxWalkSpeed` is correctly reduced in C++ (via `RecalculateMovementSpeed`) when Hunger, Thirst, or Fatigue drop below 50. However, whether this is **visually apparent** depends on the AnimBP setup:

### Problem
If locomotion animations use **root motion**, the animation drives the character's position directly and `MaxWalkSpeed` is just a cap that root motion may never exceed — the character looks like it moves at the same speed.

### Fix (do this during the animation pass)

**Step 1 — Check root motion**
Open each walk/run animation asset. If `Enable Root Motion` is checked, decide whether to disable it for locomotion (recommended for a 2D game) and use `AddMovementInput` + `MaxWalkSpeed` to drive position instead.

**Step 2 — Add `MovementSpeedMultiplier` to `ABaseCharacter`**
```cpp
// In BaseCharacter.h (public, BlueprintReadOnly):
UPROPERTY(BlueprintReadOnly, Category = "Movement")
float MovementSpeedMultiplier = 1.f;

// In RecalculateMovementSpeed():
MovementSpeedMultiplier = NeedsMult * HealthMult;
GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * MovementSpeedMultiplier;
```

**Step 3 — Wire in ABP_Player**
In `Event Blueprint Update Animation`:
```
CharacterRef → Get MovementSpeedMultiplier → store as SpeedMult
```
Multiply `SpeedMult` into the locomotion blend space sample's **Play Rate** pin, or multiply the speed input into the blend space so the character visually blends to a slower walk/run state.

---

## IK Rig — Disable During Attacks

### Problem
IK solvers fight the limb positions baked into attack montages. The legs keep
moving as if the character is walking even while a hammer swing or punch is playing,
because the IK rig is still active and overriding the montage poses.

### What does NOT work
- **`Is Playing Slot Animation`** node in the AnimBP — unreliable. Returns false for
  at least one frame after the montage starts, and the slot name must exactly match
  the slot defined inside the montage asset. Caused the hammer swing IK to stay on
  even while the animation was playing.

### Correct fix (confirmed working)
Read `bIsAttacking` directly from the character in `Event Blueprint Update Animation`:

```
Try Get Pawn Owner → Cast to BP_BaseCharacter → Get bIsAttacking → store as IsAttacking
```

Then in the **AnimGraph**, on every IK node's Alpha pin:
```
IsAttacking → NOT Boolean → B2F (bool to float) → Alpha pin on IK node
```
Result: `0.0` (IK off) while attacking, `1.0` (IK on) otherwise.

### Why `bIsAttacking` is reliable
- Set to `true` in C++ **before** `PlayAnimMontage` is called — no 1-frame lag
- Reset to `false` by `OnAttackMontageEnded` — fires exactly when the montage ends,
  regardless of how long the animation is
- **Do NOT use a fixed cooldown timer for this** — a 0.7s timer re-enables IK
  mid-animation for long montages (e.g. hammer swing > 0.7s). The timer is only
  a fallback when no montage asset is assigned.

---

## Attack Montages

| Montage | Asset | Assigned on |
|---|---|---|
| Weapon (hammer/sword) | `Content/MyContent/Animation/Stable_Sword_Outward_Slashtest_Montage` | Weapon BP (e.g. `BP_WeaponHammer`) → `AttackMontage` |
| Unarmed punch | `Content/MyContent/Animation/Cross_Punch_Anim_Montage` | `BP_BaseCharacter` → `UnarmedAttackMontage` |
| Death | TBD | `BP_BaseCharacter` → `DeathMontage` |

### Required notifies on each weapon attack montage

Add these C++ AnimNotify classes in the Montage editor notify track:

| Notify class | Where to place | Effect |
|---|---|---|
| `AnimNotify_BeginAttack` | First frame of the swing arc | Opens weapon hitbox |
| `AnimNotify_EndAttack` | Last frame of swing arc (before recovery) | Closes weapon hitbox |

Unarmed montage does **not** need notifies — hit detection uses a delayed sphere sweep.

### Slot setup
Each montage must use the **Default Slot** (or whichever full-body slot the AnimBP
slot node is named). Mismatch between the montage's slot and the AnimGraph slot node
will cause the animation to play silently with no visible pose change.

---

## How `bIsAttacking` flows end-to-end

```
LMB pressed
  → OnAttackPressed() sets bIsAttacking = true
  → PlayAnimMontage() returns duration > 0
  → OnMontageEnded delegate bound (timer NOT started)
  → Animation plays, IK disabled (AnimBP reads bIsAttacking = true)
  → Montage finishes
  → OnAttackMontageEnded() fires
  → bIsAttacking = false
  → IK re-enables on next AnimBP tick
```

If no montage asset is assigned (returns duration = 0), a fallback timer
(`AttackCooldownDuration = 0.7s`) resets `bIsAttacking` so the player is never
permanently locked out of attacking.
