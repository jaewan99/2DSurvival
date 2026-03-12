// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/DamageableInterface.h"
#include "Components/NeedsComponent.h"
#include "BaseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInteractionComponent;
class UInventoryComponent;
class UHealthComponent;
class UHotbarComponent;
class UItemDefinition;
class UInputAction;
class UInputMappingContext;
class AWeaponBase;
class AFlashlightActor;
class ADraggableProp;
class UHealthHUDWidget;
class UHotbarWidget;
class UCraftingComponent;
class UCraftingWidget;
class UNeedsWarningWidget;
class UStreetHUDWidget;
class UDialogueWidget;
class ANPCActor;
class USoundBase;
class UPostProcessComponent;
class UMapWidget;
class UJournalComponent;
class UJournalWidget;
class UStatusEffectComponent;
class UStatusEffectWidget;
class USkillComponent;
class USkillHUDWidget;
class APlaceableActor;
class UMaterialInterface;
class UNoiseEmitterComponent;

enum class EBodyPart : uint8;

UCLASS()
class TWODSURVIVAL_API ABaseCharacter : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* SideViewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UInteractionComponent* InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hotbar")
	UHotbarComponent* HotbarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	UCraftingComponent* CraftingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Needs")
	UNeedsComponent* NeedsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mood")
	UPostProcessComponent* MoodPostProcess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Journal")
	UJournalComponent* JournalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StatusEffects")
	UStatusEffectComponent* StatusEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skills")
	USkillComponent* SkillComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Noise")
	UNoiseEmitterComponent* NoiseEmitterComponent;

	// Assign IA_Interact in the Blueprint child class Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Interact;

	// Assign IA_ToggleInventory in the Blueprint child class Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_ToggleInventory;

	// Assign IA_ToggleHealthUI in the Blueprint child class Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_ToggleHealthUI;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Block jumping, crouching, and un-crouching while an attack montage is playing.
	// All three are ACharacter virtuals — Blueprint bindings call through to these automatically.
	virtual void Jump() override;
	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;

	/** Move the character left/right and rotate to face the movement direction.
	 *  @param Value  Positive = right, Negative = left, 0 = no movement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveRight(float Value);

	/**
	 * Increments the UI-open counter and shows the mouse cursor when the first UI panel opens.
	 * Call this whenever any overlay UI (inventory, crafting, health HUD, etc.) becomes visible.
	 * Pair every ShowUICursor() call with a matching HideUICursor() call on close.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowUICursor();

	/**
	 * Decrements the UI-open counter. Hides the mouse cursor only when ALL UI panels have closed.
	 * Safe to call extra times — counter will not go below zero.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideUICursor();

	/**
	 * Stores the currently visible item tooltip so it can be force-removed when UI panels close.
	 * Call this from WBP_InventorySlot's OnMouseEnter after creating the tooltip widget.
	 * Pass null to clear the stored reference without removing anything.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RegisterTooltip(UUserWidget* Tooltip);

	/**
	 * Force-removes the stored tooltip from the viewport.
	 * Safe to call when no tooltip is active or after it was already removed by OnMouseLeave.
	 * Call this from CloseContainerInventory and ToggleInventory instead of ClearContainerTooltips.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ClearTooltip();

	/**
	 * Called when the player presses the Toggle Inventory key (I).
	 * Override in BP_BaseCharacter to show/hide the inventory widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void ToggleInventory();
	virtual void ToggleInventory_Implementation() {}

	/**
	 * Called when the player presses the Toggle Health UI key (H).
	 * Creates/destroys the Health HUD widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUD")
	void ToggleHealthUI();
	virtual void ToggleHealthUI_Implementation();

	/**
	 * Called when the player presses the Crafting key (C).
	 * Creates/destroys the Crafting widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crafting")
	void ToggleCrafting();
	virtual void ToggleCrafting_Implementation();

	// Closes the crafting UI if open. Called when the player walks out of range.
	// Safe to call when already closed — does nothing.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Crafting")
	void CloseCrafting();
	virtual void CloseCrafting_Implementation();

	/**
	 * Locks movement and tells NeedsComponent to restore Fatigue instead of draining it.
	 * Called by ABedActor when the player presses E on a bed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Needs")
	void StartSleeping();
	virtual void StartSleeping_Implementation();

	/**
	 * Unlocks movement and stops sleeping. Recalculates movement speed.
	 * Called by ABedActor on second press or when Fatigue reaches 100.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Needs")
	void StopSleeping();
	virtual void StopSleeping_Implementation();

	/**
	 * Called when a container interaction completes (e.g. hold E on a chest).
	 * ContainerComp is the inventory component on the container actor.
	 * Override in BP_BaseCharacter to show the player + container panels side by side.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void OpenContainerInventory(UInventoryComponent* ContainerComp, AActor* ContainerActor);
	virtual void OpenContainerInventory_Implementation(UInventoryComponent* ContainerComp, AActor* ContainerActor) {}

	/**
	 * Called to close the container panel (e.g. player walks away).
	 * Override in BP_BaseCharacter to hide the container panel and restore input mode.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void CloseContainerInventory();
	virtual void CloseContainerInventory_Implementation() {}

	/**
	 * Consume a usable item from the given inventory slot.
	 * Checks that the slot holds a Consumable, applies the effect, and removes one unit.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void UseItem(int32 SlotIndex, UInventoryComponent* FromInventory);
	virtual void UseItem_Implementation(int32 SlotIndex, UInventoryComponent* FromInventory);

	/**
	 * Equips a weapon from the given inventory slot.
	 * Spawns the weapon actor from ItemDef->WeaponActorClass and attaches it to WeaponSocketName.
	 * Automatically unequips any previously equipped weapon first.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void EquipItem(int32 SlotIndex, UInventoryComponent* FromInventory);
	virtual void EquipItem_Implementation(int32 SlotIndex, UInventoryComponent* FromInventory);

	/**
	 * Unequips the current weapon — detaches and destroys the weapon actor.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Weapon")
	void UnequipWeapon();
	virtual void UnequipWeapon_Implementation();

	/**
	 * Unequips the flashlight — detaches and destroys the flashlight actor.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Flashlight")
	void UnequipFlashlight();
	virtual void UnequipFlashlight_Implementation();

	/**
	 * IDamageable implementation — called by enemy weapon hitboxes or unarmed sweeps.
	 * Damages the Body part. Override in Blueprint for more granular hit location logic.
	 */
	virtual void TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource) override;

	/**
	 * Fired from C++ when Head or Body health reaches 0.
	 * Override in BP_BaseCharacter to show a death / respawn screen.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnPlayerDied();

	// Set to true by Blueprint when a traversal action (vault, climb, etc.) begins.
	// Set back to false when traversal completes or is interrupted.
	// While true, attack input is blocked so traversal cannot be cancelled mid-move.
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	bool bIsTraversing = false;

	// When true, player-driven horizontal movement input is ignored (hold interaction in progress).
	// Gravity and physics still apply normally.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bMovementLocked = false;

	// Walk speed stored at BeginPlay — used to restore speed after leg-damage penalty recalculation.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float BaseWalkSpeed = 0.f;

	// Socket name on the character mesh where the weapon attaches. Set in BP_BaseCharacter defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponSocketName = FName("WeaponSocket");
	// Currently equipped weapon actor. Null if nothing is equipped.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AWeaponBase> EquippedWeapon;

	// Socket name on the character mesh where the flashlight attaches.
	// Create this socket in the Skeleton editor and set the name in BP_BaseCharacter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	FName FlashlightSocketName = FName("FlashlightSocket");

	// Currently equipped flashlight actor. Null if no flashlight is equipped.
	UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<AFlashlightActor> EquippedFlashlight;

	// The prop currently being dragged. Null when not dragging.
	UPROPERTY(BlueprintReadOnly, Category = "Drag")
	TObjectPtr<ADraggableProp> GrabbedProp;

	/**
	 * Begin dragging a prop — locks interaction focus to it and reduces walk speed.
	 * Called from ADraggableProp::OnInteract when the player presses E near the prop.
	 */
	void StartDrag(ADraggableProp* Prop);

	/**
	 * Release the currently dragged prop — restores walk speed and unlocks interaction focus.
	 * Called from ADraggableProp::OnInteract on second E press, or on death/sleep.
	 */
	void ReleaseDrag();

	// Montage played when attacking unarmed (punch). Assign in BP_BaseCharacter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Unarmed")
	TObjectPtr<UAnimMontage> UnarmedAttackMontage;

	// Damage dealt by an unarmed punch.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Unarmed")
	float UnarmedDamage = 5.f;

	// Seconds after the punch starts before the hit is checked (matches the wind-up).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Unarmed")
	float UnarmedHitDelay = 0.2f;

	// Forward reach of the punch sweep (cm).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Unarmed")
	float UnarmedHitRange = 80.f;

	// Radius of the punch sphere sweep (cm).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Unarmed")
	float UnarmedHitRadius = 40.f;

	// How long after pressing attack before another attack can begin.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float AttackCooldownDuration = 0.7f;

	// True while an attack is in progress — prevents queuing new attacks.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	// Montage played when the player dies (Head or Body reaches 0). Assign in BP_BaseCharacter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** How many times faster in-game time runs while the player is sleeping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Needs")
	float SleepTimeScale = 20.f;

	// ── Sound effects ──────────────────────────────────────────────────────────
	// Assign Sound Wave / Sound Cue assets in BP_BaseCharacter Details panel.

	/** Played when any interactable (E key press) interaction begins. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_Interact;

	/** Played when the inventory is opened. Called from Blueprint's ToggleInventory override. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_InventoryOpen;

	/** Played when the inventory is closed. Called from Blueprint's ToggleInventory override. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_InventoryClose;

	/** Played when the crafting UI opens. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_CraftOpen;

	/** Played when the crafting UI closes. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_CraftClose;

	/** Played when the dialogue widget opens. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_DialogueOpen;

	/** Played when the dialogue widget closes. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_DialogueClose;

	/** Played when the Health HUD is toggled. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_HealthHUDToggle;

	/**
	 * Plays a 2D (non-positional) UI sound. Safe to call with null (no-op).
	 * Call this from Blueprint's ToggleInventory override to play SFX_InventoryOpen/Close.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayUISound(USoundBase* Sound);

	// ── Save ───────────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save")
	FString SaveSlotName = TEXT("SaveSlot0");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save")
	int32 SaveUserIndex = 0;

	/** Save all player state to disk. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Save")
	void SaveGame();
	virtual void SaveGame_Implementation();

	/** Load player state from disk. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Save")
	void LoadGame();
	virtual void LoadGame_Implementation();

	// Widget classes — assign in BP_BaseCharacter Details panel.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHealthHUDWidget> HealthHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHotbarWidget> HotbarWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCraftingWidget> CraftingWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNeedsWarningWidget> NeedsWarningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UStreetHUDWidget> StreetHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UDialogueWidget> DialogueWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMapWidget> MapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UJournalWidget> JournalWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UStatusEffectWidget> StatusEffectWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USkillHUDWidget> SkillHUDWidgetClass;

	void ToggleMap();
	void ToggleJournal();
	void ToggleSkillHUD();

	// ── Placement mode ─────────────────────────────────────────────────────────

	/**
	 * Enters placement mode for the item at the given slot.
	 * Spawns a ghost APlaceableActor that follows the mouse cursor (XZ plane, Y=PlayerY).
	 * Does nothing if the item has no PlaceableClass or if already in placement mode.
	 * Called from the inventory context menu "Place" button (Blueprint → C++ call).
	 */
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void PlaceItem(int32 SlotIndex, UInventoryComponent* FromInventory);

	/**
	 * Confirms the current placement: finalizes the ghost actor, removes the item from
	 * inventory, and exits placement mode. No-op if ghost position is blocked.
	 * Bound to LMB (IA_Attack gate) while bIsInPlacementMode is true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void ConfirmPlacement();

	/**
	 * Cancels placement mode: destroys the ghost actor and returns to normal play.
	 * Bound to the Escape key (IA_PlaceCancel) while in placement mode.
	 */
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void CancelPlacement();

	/** True while the player is previewing a placement position. */
	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	bool bIsInPlacementMode = false;

	/**
	 * Snap grid size in cm. Placed actors snap to multiples of this value on X and Z.
	 * Tune in BP_BaseCharacter Details → Placement.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement",
		meta = (ClampMin = "1.0"))
	float PlacementGridSize = 50.f;

	/**
	 * Semi-transparent material applied to the ghost when the placement position is clear.
	 * Create a simple translucent material with a green tint and assign here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UMaterialInterface> PlacementValidMaterial;

	/**
	 * Semi-transparent material applied to the ghost when the placement position is blocked.
	 * Create a simple translucent material with a red tint and assign here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UMaterialInterface> PlacementInvalidMaterial;

	/**
	 * Opens the dialogue widget for the given NPC.
	 * Called by ANPCActor::OnInteract when the player presses E.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue")
	void OpenDialogue(ANPCActor* NPC);
	virtual void OpenDialogue_Implementation(ANPCActor* NPC);

	/**
	 * Closes the dialogue widget if open. Called when player walks away or presses Close.
	 * Safe to call when already closed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue")
	void CloseDialogue();
	virtual void CloseDialogue_Implementation();

private:
	// Maps ItemID → ItemDefinition for fast lookup during load. Built in BeginPlay via AssetRegistry scan.
	UPROPERTY()
	TMap<FName, UItemDefinition*> ItemDefMap;

	UItemDefinition* FindItemDefByID(FName ItemID) const;

	// Bound to HealthComponent->OnBodyPartDamaged.
	// Applies movement speed penalties for leg damage and triggers death for Head/Body.
	UFUNCTION()
	void OnBodyPartDamaged(EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken);

	// Bound to HealthComponent->OnDeath.
	// Disables input, plays DeathMontage, and fires the OnPlayerDied BlueprintImplementableEvent.
	UFUNCTION()
	void HandlePlayerDeath();

	// Programmatic input actions — created at runtime in CreateInputActions(), no editor assignment needed.
	UPROPERTY() UInputAction* IA_HotbarSlot1;
	UPROPERTY() UInputAction* IA_HotbarSlot2;
	UPROPERTY() UInputAction* IA_HotbarSlot3;
	UPROPERTY() UInputAction* IA_HotbarSlot4;
	UPROPERTY() UInputAction* IA_HotbarSlot5;
	UPROPERTY() UInputAction* IA_HotbarSlot6;
	UPROPERTY() UInputAction* IA_HotbarScrollUp;
	UPROPERTY() UInputAction* IA_HotbarScrollDown;
	UPROPERTY() UInputAction* IA_SaveGameAction;
	UPROPERTY() UInputAction* IA_LoadGameAction;
	UPROPERTY() UInputAction* IA_Attack;
	UPROPERTY() UInputAction* IA_ToggleMap;
	UPROPERTY() UInputAction* IA_ToggleJournal;
	UPROPERTY() UInputAction* IA_ToggleSkillHUD;
	UPROPERTY() UInputAction* IA_PlaceCancel;
	UPROPERTY() UInputMappingContext* GameplayIMC;

	/** Creates all programmatic input actions and mapping context at runtime. */
	void CreateInputActions();

	// Hotbar input helpers
	void SelectHotbarSlot1();
	void SelectHotbarSlot2();
	void SelectHotbarSlot3();
	void SelectHotbarSlot4();
	void SelectHotbarSlot5();
	void SelectHotbarSlot6();
	void HotbarScrollUp();
	void HotbarScrollDown();
	void OnSaveGamePressed();
	void OnLoadGamePressed();
	// Interact wrappers — gate on !bIsAttacking before forwarding to InteractionComponent.
	void OnInteractStarted();
	void OnInteractCompleted();

	void OnAttackPressed();
	void ResetAttack();
	void PerformUnarmedHit();

	// Bound to UAnimInstance::OnMontageEnded after each attack.
	// Resets bIsAttacking (re-enables IK) exactly when the montage finishes,
	// rather than after a fixed AttackCooldownDuration timer.
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Recalculates MaxWalkSpeed from BaseWalkSpeed × health multiplier × needs multiplier.
	void RecalculateMovementSpeed();

	// Bound to NeedsComponent->OnNeedChanged — triggers speed recalculation on need change.
	UFUNCTION()
	void OnNeedChanged(ENeedType NeedType, float CurrentValue, float MaxValue, bool bIsWarning);

	// Bound to NeedsComponent->OnMoodChanged — applies post-process desaturation effect.
	UFUNCTION()
	void OnMoodChanged(float NewMood);

	// Bound to CraftingComponent->OnCraftingChanged — boosts mood on successful craft.
	UFUNCTION()
	void OnCraftSucceeded();

	// Widget instances
	UPROPERTY()
	UHealthHUDWidget* HealthHUDInstance;

	UPROPERTY()
	UHotbarWidget* HotbarWidgetInstance;

	UPROPERTY()
	UCraftingWidget* CraftingWidgetInstance;

	UPROPERTY()
	UNeedsWarningWidget* NeedsWarningInstance;

	UPROPERTY()
	UStreetHUDWidget* StreetHUDInstance;

	UPROPERTY()
	UDialogueWidget* DialogueWidgetInstance;

	UPROPERTY()
	UMapWidget* MapWidgetInstance;

	UPROPERTY()
	UJournalWidget* JournalWidgetInstance;

	UPROPERTY()
	UStatusEffectWidget* StatusEffectWidgetInstance;

	UPROPERTY()
	USkillHUDWidget* SkillHUDInstance;

	UFUNCTION()
	void OnStatusEffectsChanged();

	/** Scans all UItemDefinition assets via AssetRegistry and builds ItemDefMap. */
	void ScanItemDefinitions();

	// Dynamic material instances for all slots on the character mesh.
	// Created in BeginPlay so we can push the CharacterForward parameter each tick.
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CharacterMIDs;

	// Last forward vector pushed to the materials — skips redundant updates.
	FVector LastForwardForMaterial = FVector::ZeroVector;

	// Active ghost actor while in placement mode. Null when not placing.
	UPROPERTY()
	TObjectPtr<APlaceableActor> PlacementGhost;

	// Item definition being placed — used to finalize and remove from inventory on confirm.
	UPROPERTY()
	TObjectPtr<UItemDefinition> PlacementItemDef;

	// Inventory the placeable item came from — used to remove the item on confirm.
	UPROPERTY()
	TObjectPtr<UInventoryComponent> PlacementFromInventory;

	// Slot index in PlacementFromInventory where the placeable item lives.
	int32 PlacementSlotIndex = -1;

	// Cached per-tick flag: true when the ghost's current position is unobstructed.
	bool bPlacementIsValid = false;

	// Moves PlacementGhost to the mouse-projected XZ position and updates its tint.
	void UpdatePlacementGhost();

	// Returns the world XZ position under the mouse projected onto Y=PlayerY plane.
	// Returns false if the deproject ray is parallel to the plane (edge case).
	bool GetMousePlacementLocation(FVector& OutLocation) const;

	FTimerHandle AttackCooldownTimer;
	FTimerHandle UnarmedHitTimer;

	// True while the player is dragging a prop.
	bool bIsDragging = false;

	// +1.f = prop is to the right of the player, -1.f = prop is to the left.
	// Determines the offset direction applied each tick in UpdateDraggedPropPosition.
	float DragGrabSide = 1.f;

	// Translates GrabbedProp to stay glued to the player's side. Called every Tick while bIsDragging.
	void UpdateDraggedPropPosition();

	// How many UI panels currently want the mouse cursor shown.
	// ShowUICursor() increments, HideUICursor() decrements.
	// Cursor is hidden only when this reaches 0.
	int32 UIOpenCount = 0;

	// The currently visible item tooltip widget, registered by WBP_InventorySlot on hover.
	// Cleared by ClearTooltip() when any UI panel closes.
	UPROPERTY()
	UUserWidget* ActiveItemTooltip = nullptr;
};
