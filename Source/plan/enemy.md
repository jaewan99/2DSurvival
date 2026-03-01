# Enemy AI System — Implementation Plan

## Overview

A zombie-like melee enemy for the 2D side-scrolling survival game.
The enemy uses a **C++ state machine** (no Behavior Trees, no NavMesh) with
**manual X-axis movement** — consistent with the project's C++-first philosophy
and the 2D movement model used by `ABaseCharacter`.

**Planned date:** 2026-03-01

---

## Enemy States

```
EEnemyState:
  Idle    → stands still, plays idle anim, waits IdleWaitTime before patrolling
  Patrol  → walks back and forth within PatrolRange of spawn point
  Chase   → runs toward player's X position (gravity handles Z naturally)
  Attack  → plays attack montage, melee hitbox active during swing window
  Dead    → loot drops, death animation, actor destroyed after DeathDelay
```

### Transitions

| From | To | Condition |
|---|---|---|
| Idle | Patrol | After `IdleWaitTime` (default 2 s) |
| Idle/Patrol | Chase | Player enters `AggroRange` and has line of sight |
| Chase | Attack | Player X-distance ≤ `AttackRange` |
| Attack | Chase | Attack montage ends (or `AttackCooldown` timer fires if no montage) |
| Chase | Idle | Player exits `AggroRange * 1.5` (hysteresis to prevent oscillation) |
| Any | Dead | `UHealthComponent::OnDeath` fires |

---

## Files

### New Files

| File | Purpose |
|---|---|
| `Public/Enemy/EnemyTypes.h` | `EEnemyState` enum + `FLootEntry` struct |
| `Public/Enemy/EnemyBase.h` | `AEnemyBase` declaration |
| `Private/Enemy/EnemyBase.cpp` | `AEnemyBase` implementation |
| `Public/World/WorldItem.h` | `AWorldItem` — interactable loot pickup actor |
| `Private/World/WorldItem.cpp` | `AWorldItem` implementation |
| `Public/UI/EnemyHealthBarWidget.h` | `UEnemyHealthBarWidget` C++ widget |
| `Private/UI/EnemyHealthBarWidget.cpp` | `UEnemyHealthBarWidget` implementation |

### No Build.cs changes needed
All required modules (`EnhancedInput`, `UMG`, `AssetRegistry`) already present.

---

## Architecture

### `EnemyTypes.h`

```cpp
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    Idle    UMETA(DisplayName="Idle"),
    Patrol  UMETA(DisplayName="Patrol"),
    Chase   UMETA(DisplayName="Chase"),
    Attack  UMETA(DisplayName="Attack"),
    Dead    UMETA(DisplayName="Dead")
};

USTRUCT(BlueprintType)
struct FLootEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UItemDefinition> ItemDef;  // What drops

    UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0.0", ClampMax="1.0"))
    float DropChance = 1.0f;             // 0–1 probability

    UPROPERTY(EditDefaultsOnly)
    int32 MinCount = 1;

    UPROPERTY(EditDefaultsOnly)
    int32 MaxCount = 1;
};
```

---

### `AEnemyBase` (ACharacter, IDamageable)

#### Components
| Component | Details |
|---|---|
| `UHealthComponent* HealthComp` | Reuse existing — single body pool (`BodyMaxHealth`) |
| `UBoxComponent* MeleeHitbox` | Attack hitbox — disabled by default, enable on swing |
| `UWidgetComponent* HealthBarComp` | World-space widget showing enemy health bar |

#### EditDefaultsOnly Properties
| Property | Type | Default | Notes |
|---|---|---|---|
| `AggroRange` | float | 800 | Detection sphere radius (cm) |
| `AttackRange` | float | 90 | Melee reach (cm) |
| `PatrolRange` | float | 400 | Max wander distance from spawn |
| `LoseAggroMultiplier` | float | 1.5 | AggroRange × this = lose-aggro distance |
| `BaseDamage` | float | 20 | Damage per hit (before player armor) |
| `AttackCooldown` | float | 1.5 | Seconds between attacks |
| `ChaseSpeed` | float | 300 | MaxWalkSpeed when chasing |
| `PatrolSpeed` | float | 150 | MaxWalkSpeed when patrolling |
| `DeathDestroyDelay` | float | 3.0 | Seconds before actor is destroyed after death |
| `AttackMontage` | UAnimMontage* | null | Zombie swing/claw animation |
| `DeathMontage` | UAnimMontage* | null | Death animation |
| `LootTable` | TArray<FLootEntry> | empty | Set in BP_EnemyBase |
| `HealthBarWidgetClass` | TSubclassOf<UUserWidget> | null | Assign WBP_EnemyHealthBar |
| `bShowHealthBarOnlyWhenDamaged` | bool | true | Collapses bar until first hit |

#### Key Methods

```
BeginPlay()
  - Store SpawnLocation (for patrol origin)
  - Set CurrentState = Idle
  - Bind HealthComp->OnDeath to OnEnemyDeath()
  - Bind MeleeHitbox->OnComponentBeginOverlap to OnMeleeHitboxOverlap()
  - Init HealthBarWidget via HealthBarComp

Tick(DeltaTime)
  - If Dead → return
  - RunCurrentState() → Idle / Patrol / Chase / Attack
  - CheckStateTransitions() → update CurrentState if needed

--- State behavior ---

RunIdle()
  - Accumulate IdleTimer
  - If IdleTimer >= IdleWaitTime → switch to Patrol

RunPatrol()
  - Move toward PatrolTarget (an X position ± PatrolRange from SpawnLocation)
  - If reached PatrolTarget → reverse direction
  - Face movement direction (SetActorRotation)

RunChase()
  - Move toward player's X position at ChaseSpeed
  - Face player direction

RunAttack()
  - On attack start: PlayAnimMontage(AttackMontage), enable hitbox, start cooldown
  - On montage end or cooldown: reset CanAttack flag

--- Transitions ---

DetectPlayer() → ABaseCharacter* (or nullptr)
  - Sphere check: all ABaseCharacter actors within AggroRange
  - Optional line-of-sight: trace to player, ignore self

CheckStateTransitions()
  - Idle/Patrol → Chase: DetectPlayer() != nullptr
  - Chase → Attack: XDistanceToPlayer <= AttackRange && CanAttack
  - Chase → Idle: player distance > AggroRange * LoseAggroMultiplier
  - Attack → Chase: done attacking

--- Combat ---

BeginMeleeAttack()
  - If hitbox already active → return (guard)
  - Enable MeleeHitbox collision
  - PlayAnimMontage
  - Start FTimerHandle to call EndMeleeAttack after SwingWindowDuration

EndMeleeAttack()
  - Disable MeleeHitbox collision

OnMeleeHitboxOverlap(...)
  - Cast hit actor to IDamageable
  - If valid && not already hit this swing → call TakeMeleeDamage(BaseDamage, this)
  - Track HitActors TSet<AActor*> to prevent multiple hits per swing

--- Death ---

OnEnemyDeath()
  - CurrentState = Dead
  - Stop all movement (GetCharacterMovement()->StopMovementImmediately())
  - bMovementLocked = true (on enemy: just stop input)
  - Play DeathMontage (if assigned)
  - SpawnLoot()
  - Collapse HealthBar
  - GetWorldTimerManager().SetTimer → DestroyAfterDelay

SpawnLoot()
  - For each FLootEntry in LootTable:
    FMath::FRand() <= DropChance → spawn AWorldItem at (Location + random offset)

TakeMeleeDamage_Implementation(Amount, Source)
  - HealthComp->ApplyDamage(EBodyPart::Body, Amount)
  - Show health bar (if bShowHealthBarOnlyWhenDamaged)
  - Apply knockback impulse toward -Source.GetActorForwardVector()
```

> **Note:** `UHealthComponent` is reused as-is but only `EBodyPart::Body` is used on
> enemies — no per-limb damage. Head/Body death still fires `OnDeath` correctly.

---

### `AWorldItem` (AActor, IInteractableInterface)

Dropped loot that lands in the world. Implements the existing `IInteractableInterface`
so the player's `UInteractionComponent` detects it automatically (press E to pick up).

#### Components
| Component | Details |
|---|---|
| `UStaticMeshComponent* Mesh` | Cube or sphere placeholder (swap in BP) |
| `USphereComponent* InteractionSphere` | Trigger for proximity detection |

#### Properties
| Property | Type | Notes |
|---|---|---|
| `ItemDef` | UItemDefinition* | Set by `AEnemyBase::SpawnLoot()` |
| `Quantity` | int32 | Set by `AEnemyBase::SpawnLoot()` |

#### Key Methods

```
Interact_Implementation(APawn* Interactor)
  - Cast Interactor to ABaseCharacter
  - Get InventoryComponent
  - TryAddItem(ItemDef, Quantity) → if success, DestroyActor

GetInteractionText_Implementation() → "Pick up [ItemDef->DisplayName] (x Quantity)"
```

---

### `UEnemyHealthBarWidget` (UUserWidget)

Simple progress bar above the enemy's head. Attached via `UWidgetComponent`.

#### BindWidget Properties
| Name | Type |
|---|---|
| `HealthBar` | UProgressBar* |

#### Methods
```
SetHealthPercent(float Percent)
  - HealthBar->SetPercent(Percent)
  - Color: green > 0.6, yellow > 0.3, red <= 0.3

NativeConstruct()
  - If bShowHealthBarOnlyWhenDamaged → collapse root
```

**Blueprint child:** `WBP_EnemyHealthBar` — just a ProgressBar named `HealthBar`.

---

## Movement (2D Manual — No NavMesh)

Enemy movement follows the same pattern as `ABaseCharacter`:

```cpp
// Chase: move toward player on X axis only
float Direction = FMath::Sign(PlayerX - GetActorLocation().X);
AddMovementInput(FVector(Direction, 0.f, 0.f), 1.f);

// Rotate to face direction (same as player rotation logic)
FRotator NewRot = Direction > 0 ? FRotator(0,0,0) : FRotator(0,180,0);
SetActorRotation(NewRot);
```

Movement constrained to X/Z plane (`bConstrainToPlane = true`, same as player character).
Gravity and jumping work natively because `ACharacter` has `UCharacterMovementComponent`.

---

## Blueprint Steps (0-3 per feature)

### `BP_EnemyBase` (child of AEnemyBase)
1. Assign skeletal mesh + ABP_Enemy (animation blueprint)
2. Assign `LootTable` entries (item definitions + chances) in Details panel
3. Assign `HealthBarWidgetClass = WBP_EnemyHealthBar`
4. Assign `AttackMontage` and `DeathMontage` in Details panel

### `WBP_EnemyHealthBar` (child of UEnemyHealthBarWidget)
1. Add a `ProgressBar` named `HealthBar` (fill horizontal, colored)

### `ABP_Enemy` (Animation Blueprint for enemy)
1. Create new AnimBP for the enemy mesh
2. Variables from `AEnemyBase`: `Speed` (float), `bIsAttacking` (bool)
3. State machine: Idle → Walk/Run (blend by speed) → Attack montage slot

---

## Integration with Existing Systems

| Existing System | How Enemy Uses It |
|---|---|
| `IDamageable` interface | Player weapon hitbox overlaps enemy → `TakeMeleeDamage_Implementation` |
| `UHealthComponent` | Enemy reuses the same component, single `EBodyPart::Body` pool |
| `IInteractableInterface` | `AWorldItem` implements it; `UInteractionComponent` detects automatically |
| `UInventoryComponent` | `AWorldItem::Interact_Implementation` calls `TryAddItem` on player inventory |
| `AWeaponBase` | NOT used — enemy has its own `UBoxComponent` hitbox (no weapon item) |

---

## Design Decisions

| Question | Decision |
|---|---|
| Jumping | **No** — zombie stays on current platform, cannot jump to reach player |
| Loot pickup | **Press E** — `AWorldItem` implements `IInteractableInterface`, existing system handles it |
| Health bar visibility | **Only when damaged** — collapsed until first hit, then stays visible |
| Aggro detection | **Sphere only** — no line trace, aggros through walls |

## Post-Implementation Adjustments

### Post-Attack Stand-Still (`PostAttackStandStillDuration`)

Added `PostAttackStandStillDuration` (EditDefaultsOnly, default 0.3s) to prevent the sliding
transition from Attack → Walk/Chase. A `bPostAttackMoveLocked` bool blocks both state transitions
and new attack initiations for `AttackCooldown + PostAttackStandStillDuration` seconds from the
start of each swing. The `PostAttackMoveTimer` handles the unlock via `OnPostAttackMoveUnlocked()`.

- `bCanAttack` resets after `AttackCooldown` (controls when the next swing can fire)
- `bPostAttackMoveLocked` resets after `AttackCooldown + PostAttackStandStillDuration` (controls when the enemy can move/transition)

### Rotation Lock During Attack (`bIsAttacking`)

`TickAttack` guards the `SetActorRotation` call with `if (!bIsAttacking)`. While a swing is
in progress, the enemy's facing direction is frozen at the angle it committed to when
`BeginMeleeAttack()` was called. This lets the player jump over the enemy mid-swing without
being tracked. Facing updates resume between attacks (when `bIsAttacking = false`).

---

## Verification Checklist

- [ ] Compile — no errors
- [ ] Enemy idles in place on spawn
- [ ] Enemy patrols left/right within patrol range
- [ ] Player walks into AggroRange → enemy chases
- [ ] Enemy swings, hitbox overlaps player → `TakeMeleeDamage` fires on player
- [ ] Player hits enemy with weapon → `TakeMeleeDamage` fires on enemy, health bar updates
- [ ] Enemy health reaches 0 → death animation plays, loot drops
- [ ] Player presses E near loot → item added to inventory, `AWorldItem` destroyed
- [ ] Enemy destroyed after `DeathDestroyDelay`
- [ ] Player runs away past lose-aggro range → enemy returns to patrol/idle
