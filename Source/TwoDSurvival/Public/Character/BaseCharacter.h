// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/DamageableInterface.h"
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
class UHealthHUDWidget;
class UHotbarWidget;
class UCraftingComponent;
class UCraftingWidget;

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

	// Widget instances
	UPROPERTY()
	UHealthHUDWidget* HealthHUDInstance;

	UPROPERTY()
	UHotbarWidget* HotbarWidgetInstance;

	UPROPERTY()
	UCraftingWidget* CraftingWidgetInstance;

	/** Scans all UItemDefinition assets via AssetRegistry and builds ItemDefMap. */
	void ScanItemDefinitions();

	FTimerHandle AttackCooldownTimer;
	FTimerHandle UnarmedHitTimer;

	// How many UI panels currently want the mouse cursor shown.
	// ShowUICursor() increments, HideUICursor() decrements.
	// Cursor is hidden only when this reaches 0.
	int32 UIOpenCount = 0;

	// The currently visible item tooltip widget, registered by WBP_InventorySlot on hover.
	// Cleared by ClearTooltip() when any UI panel closes.
	UPROPERTY()
	UUserWidget* ActiveItemTooltip = nullptr;
};
