# Draggable Props System

## Overview

Wheeled / slideable props (chairs, wheelie bins, crates) that the player can grab
with E and push left/right. Releasing E drops the prop.

---

## Disambiguation Strategy (same E key as other interactions)

**Problem**: The player can stand next to a chair that is touching a bed — both
are interactable. Pressing E must grab the chair, not sleep in the bed.

**Solution — two layers**:

1. **Priority tier in `UpdateFocusedInteractable`**
   - `ADraggableProp` is always preferred over any other `IInteractable` when both
     are in the detection sphere, regardless of distance.
   - Among same-tier actors, closest wins.
   - This is a general fix for the "duplicate interaction" problem the project
     will encounter elsewhere too.

2. **Focus lock while dragging**
   - Once a prop is grabbed, `InteractionComponent` locks `FocusedInteractable`
     to that prop (`bFocusLocked = true`, `LockedFocusActor = GrabbedProp`).
   - `UpdateFocusedInteractable` is a no-op while locked — the bed can enter
     proximity, it will never steal focus.
   - Pressing E while locked → `CompleteInteraction` → `OnInteract` on the prop
     → prop sees `Interactor->GrabbedProp == this` → calls `Interactor->ReleaseDrag()`.
   - On release: `UnlockFocus()` restores normal focus selection.

---

## Wall Behaviour (open question)

When the dragged prop hits a wall:
- If **player also stops**: set `GetCharacterMovement()->Velocity.X = 0` when
  sweep returns a blocking hit. Feels like pushing against resistance.
- If **player walks free**: prop stays against wall, player detaches naturally.

*Ask user to confirm before implementing — default implementation will stop the player.*

---

## New Files

### `Public/World/DraggableProp.h` / `Private/World/DraggableProp.cpp`

```cpp
class ADraggableProp : public AActor, public IInteractable
{
    UPROPERTY(VisibleAnywhere) USceneComponent* RootComp;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Mesh;
    UPROPERTY(VisibleAnywhere) UBoxComponent* CollisionBox;   // BlockAll — blocks world
    UPROPERTY(VisibleAnywhere) UBoxComponent* InteractionBox; // OverlapAllDynamic — trigger

    // Speed penalty while this prop is being dragged (multiplied into MaxWalkSpeed).
    UPROPERTY(EditDefaultsOnly, Category = "Drag")
    float DragSpeedMultiplier = 0.7f;

    // Half-extent of the prop on the X axis — used to keep player capsule outside the mesh.
    UPROPERTY(EditDefaultsOnly, Category = "Drag")
    float PropHalfExtentX = 50.f;

    // IInteractable
    EInteractionType GetInteractionType_Implementation() → Instant
    float GetInteractionDuration_Implementation() → 0
    FText GetInteractionPrompt_Implementation()
        → "Push" when not grabbed, "Release" when Interactor->GrabbedProp == this
    void OnInteract_Implementation(ABaseCharacter* Interactor)
        → if Interactor->GrabbedProp == this: Interactor->ReleaseDrag()
          else: Interactor->StartDrag(this)
};
```

**Collision setup in constructor**:
- `CollisionBox` → `SetCollisionProfileName("BlockAll")`
- `InteractionBox` → `SetCollisionProfileName("OverlapAllDynamic")`
- Bind `InteractionBox::OnComponentBeginOverlap/EndOverlap` (UInteractionComponent detects it)

---

## Modified Files

### `InteractionComponent.h / .cpp`

Add focus-lock API:

```cpp
// Lock focused interactable to a specific actor (used during drag).
void LockFocus(AActor* Actor);
void UnlockFocus();

private:
    bool bFocusLocked = false;
    UPROPERTY() AActor* LockedFocusActor = nullptr;
```

`UpdateFocusedInteractable` changes:
1. If `bFocusLocked` → `FocusedInteractable = LockedFocusActor`; return early.
2. Otherwise, sort `NearbyInteractables`:
   - **Tier 1**: actors that are `ADraggableProp`
   - **Tier 2**: all other interactables
   - Within each tier, pick the closest.

```cpp
void UInteractionComponent::UpdateFocusedInteractable()
{
    if (bFocusLocked) { FocusedInteractable = LockedFocusActor; return; }

    AActor* BestDraggable = nullptr;
    AActor* BestOther     = nullptr;
    float   BestDragDist  = MAX_FLT;
    float   BestOtherDist = MAX_FLT;
    FVector OwnerLoc = GetOwner()->GetActorLocation();

    for (AActor* Actor : NearbyInteractables)
    {
        float Dist = FVector::Dist(OwnerLoc, Actor->GetActorLocation());
        if (Actor->IsA<ADraggableProp>())
        {
            if (Dist < BestDragDist) { BestDraggable = Actor; BestDragDist = Dist; }
        }
        else
        {
            if (Dist < BestOtherDist) { BestOther = Actor; BestOtherDist = Dist; }
        }
    }
    FocusedInteractable = BestDraggable ? BestDraggable : BestOther;
}
```

### `BaseCharacter.h / .cpp`

New properties:

```cpp
UPROPERTY() ADraggableProp* GrabbedProp = nullptr;
bool bIsDragging = false;
float DragGrabSide = 1.f; // +1 = prop is to player's right, -1 = to player's left
```

New functions:

```cpp
void StartDrag(ADraggableProp* Prop);
void ReleaseDrag();
void UpdateDraggedPropPosition();   // called from Tick while bIsDragging
```

#### `StartDrag(ADraggableProp* Prop)`

```cpp
GrabbedProp   = Prop;
bIsDragging   = true;
// Determine side: positive = prop is to the right of player
DragGrabSide  = (Prop->GetActorLocation().X > GetActorLocation().X) ? 1.f : -1.f;
// Apply speed penalty
GetCharacterMovement()->MaxWalkSpeed *= Prop->DragSpeedMultiplier;
// Lock interaction focus
InteractionComponent->LockFocus(Prop);
```

#### `ReleaseDrag()`

```cpp
bIsDragging  = false;
GrabbedProp  = nullptr;
RecalculateMovementSpeed();          // restores correct MaxWalkSpeed
InteractionComponent->UnlockFocus();
```

#### `UpdateDraggedPropPosition()` (called from Tick)

```cpp
if (!bIsDragging || !GrabbedProp) return;

float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
float TargetX = GetActorLocation().X
              + DragGrabSide * (CapsuleRadius + GrabbedProp->PropHalfExtentX);

FVector NewLoc = GrabbedProp->GetActorLocation();
NewLoc.X = TargetX;

FHitResult Hit;
bool bMoved = GrabbedProp->SetActorLocation(NewLoc, /*bSweep=*/true, &Hit);
if (!bMoved || Hit.bBlockingHit)
{
    // Prop hit a wall — stop player lateral movement too (pushing into wall feel)
    GetCharacterMovement()->Velocity.X = 0.f;
}
```

*Z is never changed — prop stays at its current floor elevation. No stair pushing.*

---

## Blueprint Steps (0-2 actions)

1. Create `BP_DraggableProp` (parent: `ADraggableProp`) — assign mesh and
   adjust `CollisionBox` / `InteractionBox` extents in Details to match the mesh.
2. Set `DragSpeedMultiplier` and `PropHalfExtentX` per prop in BP class defaults.

No Blueprint logic. Zero nodes.

---

## Edge Cases & Notes

- **Auto-release on death / sleep / distance**: call `ReleaseDrag()` from
  `HandlePlayerDeath()`, `StartSleeping()`, and if `GrabbedProp` somehow leaves
  the detection sphere (add check in `OnDetectionEndOverlap`).
- **Prop falls off ledge while grabbed**: Z is locked during drag; when released
  the prop's physics/gravity takes over. (No gravity on props during drag — they
  are kinematic AActor, not Pawn, so no gravity unless we add a floor-snap trace.)
- **Prop collision with player capsule**: offset calculation `CapsuleRadius + PropHalfExtentX`
  keeps prop edge exactly at capsule edge — no overlap, no jitter.
- **Multiple draggable props nearby**: priority picks the closest `ADraggableProp`
  (same tier). Player must walk to the other one to grab it.
- **General duplicate interactable rule**: the tiered `UpdateFocusedInteractable`
  is a general fix — if any other priority cases arise (e.g. WorldItem inside a
  CraftingTable box), add more tiers here.
