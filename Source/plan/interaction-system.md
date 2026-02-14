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

1. Create `EInteractionType` enum and `IInteractable` interface
2. Create `UInteractionComponent` on BaseCharacter
   - Overlap detection logic
   - Hold timer with movement lock/unlock
   - Blueprint events for UI binding
3. Create base interactable actors (door, loot crate) implementing `IInteractable`
4. Create progress bar widget (UMG) bound to the interaction component
5. Hook up input (E key) in Blueprint

## File Structure

```
Source/TwoDSurvival/
├── Public/
│   ├── Interaction/
│   │   ├── InteractionTypes.h          (enum)
│   │   ├── InteractableInterface.h     (interface)
│   │   └── InteractionComponent.h      (actor component)
│   └── Character/
│       └── BaseCharacter.h             (add InteractionComponent)
├── Private/
│   ├── Interaction/
│   │   ├── InteractionComponent.cpp
│   └── Character/
│       └── BaseCharacter.cpp
```
