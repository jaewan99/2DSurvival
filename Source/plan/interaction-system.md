# Interaction System Plan

## Overview

A flexible interaction system where different objects have different interaction behaviors (instant, hold-to-interact, etc.). Detection is proximity-based using overlap volumes.

## Interaction Types

| Type | Duration | Example | Behavior |
|------|----------|---------|----------|
| Instant | 0s | Door | Press E once, action fires immediately |
| Hold | 1.5s (configurable) | Loot/Item | Hold E for the full duration to complete |

## Rules

- **Detection**: Overlap-based (Box/Sphere collision on the interactable actor). Player enters the volume → interaction becomes available.
- **Movement lock**: When the player starts holding the interaction key, movement is disabled. Releasing the key before completion cancels the interaction and restores movement.
- **Progress UI**: Hold interactions show a progress bar that fills over the hold duration. Instant interactions do not show a progress bar.
- **Cancellation**: Releasing the key early on a hold interaction cancels it and resets progress.

## Architecture

### 1. `IInteractable` (Interface)
- `GetInteractionType()` → returns `EInteractionType` (Instant / Hold)
- `GetInteractionDuration()` → returns float (0 for instant)
- `GetInteractionPrompt()` → returns FText (e.g., "Open Door", "Loot Crate")
- `OnInteract(ABaseCharacter* Interactor)` → called when interaction completes

### 2. `EInteractionType` (Enum)
- `Instant`
- `Hold`

### 3. `UInteractionComponent` (ActorComponent on BaseCharacter)
- Detects nearby interactables via overlap events
- Tracks the current closest/focused interactable
- Handles input: press/hold E key
- Manages hold timer and progress (0.0 → 1.0)
- Disables movement on hold start, re-enables on complete/cancel
- Exposes `InteractionProgress` (float 0-1) for UI binding

### 4. Progress Bar Widget (UMG)
- Bound to `UInteractionComponent::InteractionProgress`
- Shows prompt text from `GetInteractionPrompt()`
- Only visible when in range of a hold-type interactable and actively holding

## Implementation Steps

1. Create `EInteractionType` enum and `IInteractable` interface ✅
2. Create `UInteractionComponent` on BaseCharacter ✅
   - Overlap detection logic
   - Hold timer with movement lock/unlock
   - Blueprint events for UI binding
3. Create base interactable actors (door, loot crate) implementing `IInteractable` ✅ (Blueprint actors)
4. Create progress bar widget (UMG) bound to the interaction component ✅
5. Hook up input (E key) in C++ via EnhancedInput ✅

## File Structure

```
Source/TwoDSurvival/
├── Public/
│   ├── Interaction/
│   │   ├── InteractionTypes.h          (enum)
│   │   ├── InteractableInterface.h     (interface)
│   │   └── InteractionComponent.h      (actor component)
│   └── Character/
│       └── BaseCharacter.h             (add InteractionComponent + IA_Interact)
├── Private/
│   ├── Interaction/
│   │   ├── InteractableInterface.cpp   (required by UHT — minimal, just include)
│   │   ├── InteractionComponent.cpp
│   └── Character/
│       └── BaseCharacter.cpp
```

## Implementation Notes (for future reference)

### Input binding
- `IA_Interact` is an `EditAnywhere` UPROPERTY on `ABaseCharacter` — assign the asset in BP_BaseCharacter Details panel
- Bound in `SetupPlayerInputComponent` via `UEnhancedInputComponent::BindAction`
  - `ETriggerEvent::Started` → `StartInteract`
  - `ETriggerEvent::Completed` → `StopInteract`
- **IA_Interact must have NO triggers configured** (leave Triggers array empty). The C++ component handles all hold timing internally. Adding a Hold trigger on the IA breaks cancellation.
- The IMC (Input Mapping Context) must still be added in Blueprint BeginPlay via `Add Mapping Context` on the Enhanced Input subsystem — that's separate from the action binding.

### Detection sphere
- Created dynamically in `UInteractionComponent::BeginPlay` using `NewObject<USphereComponent>`
- Attached to owner's root, registered, then overlap delegates bound
- Profile: `OverlapAllDynamic`, `QueryOnly`, `HiddenInGame`
- Default radius: `150` units — adjustable via `DetectionRadius` UPROPERTY

### UI widget (WBP_InteractionHUD)
- Uses **Add to Viewport** (not Widget Component — binding evaluation is unreliable in Widget Components)
- Created and added in BP_BaseCharacter BeginPlay
- Percent binding: `Get Player Character (index 0) → Cast to BP_BaseCharacter → Get InteractionComponent → Get InteractionProgress`
- Text binding: same chain but reads `InteractionPrompt`
- Default UMG text color is white — change to a visible color or add a dark background

### Interactable actors (Blueprint)
- Plain Blueprint Actor (no special parent needed)
- Add `Interactable` interface via **Class Settings → Interfaces → Add**
- Override all four interface functions: `GetInteractionType`, `GetInteractionDuration`, `GetInteractionPrompt`, `OnInteract`
- Add a collision shape (Box or Sphere), set profile to `OverlapAll`, enable `Generate Overlap Events`
- The collision volume is what the player's detection sphere overlaps — size it to match the intended interaction range

### Key gotchas
- `StartInteract` has an `if (bIsInteracting) return` guard — prevents the timer restarting if input fires more than once mid-hold
- Movement is locked via `DisableMovement()` on hold start, restored via `SetMovementMode(MOVE_Walking)` on complete/cancel
- `StopInteract` is a no-op if `bIsInteracting` is false — safe to call on every key release including after natural completion
