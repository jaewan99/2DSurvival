# TwoDSurvival - Progress Tracker

## Completed Steps

| # | Task | Date | Notes |
|---|------|------|-------|
| 1 | Initial project setup | 2026-02-14 | Created Unreal Engine project with default structure |
| 2 | GitHub Actions setup | 2026-02-14 | Added Claude Code workflow for issue handling (no code review) |
| 3 | 2D side-view camera setup | 2026-02-14 | SpringArm + Camera on BaseCharacter, side-scroll view, plane-constrained movement, smooth camera lag |
| 4 | 2D movement with character rotation | 2026-02-14 | MoveRight() BlueprintCallable function — moves left/right in world space, rotates character to face movement direction via SetControlRotation |
| 5 | Interaction system | 2026-02-15 | IInteractable interface, EInteractionType enum, UInteractionComponent on BaseCharacter. Proximity detection via dynamic sphere. Hold timer with movement lock, progress 0→1. Input (E key) bound in C++ via EnhancedInput (IA_Interact). UMG progress bar uses Add to Viewport with Get Player Character binding. |
