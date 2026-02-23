// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInteractionComponent;
class UInventoryComponent;
class UInputAction;
class AWeaponBase;

UCLASS()
class TWODSURVIVAL_API ABaseCharacter : public ACharacter
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	float Health = 100.f;

	// Assign IA_Interact in the Blueprint child class Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Interact;

	// Assign IA_ToggleInventory in the Blueprint child class Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_ToggleInventory;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Move the character left/right and rotate to face the movement direction.
	 *  @param Value  Positive = right, Negative = left, 0 = no movement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveRight(float Value);

	/**
	 * Called when the player presses the Toggle Inventory key (I).
	 * Override in BP_BaseCharacter to show/hide the inventory widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
	void ToggleInventory();
	virtual void ToggleInventory_Implementation() {}

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

	// Socket name on the character mesh where the weapon attaches. Set in BP_BaseCharacter defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponSocketName = FName("WeaponSocket");

	// Currently equipped weapon actor. Null if nothing is equipped.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AWeaponBase> EquippedWeapon;
};