// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/HotbarComponent.h"
#include "Weapon/WeaponBase.h"
#include "Save/TwoDSurvivalSaveGame.h"
#include "Kismet/GameplayStatics.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't let the controller rotate the character — we handle rotation manually in MoveRight
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Spring arm extends out to the side (along Y axis) for a side-view camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	SpringArm->TargetArmLength = 800.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.f;

	// Camera attached to the spring arm
	SideViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	SideViewCamera->bUsePawnControlRotation = false;

	// Interaction component — handles proximity detection and hold-timer logic
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

	// Inventory component — holds the player's items
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Health component — per-body-part health pools
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Hotbar component — quick-select item slots
	HotbarComponent = CreateDefaultSubobject<UHotbarComponent>(TEXT("HotbarComponent"));

	// Don't auto-rotate toward movement direction — MoveRight handles rotation
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Lock character movement to a 2D plane (X/Z only)
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Store the default walk speed so leg damage can scale it relative to this base.
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// React to body part damage — adjust movement speed and trigger death.
	HealthComponent->OnBodyPartDamaged.AddDynamic(this, &ABaseCharacter::OnBodyPartDamaged);

	// Build item lookup map for save/load.
	for (UItemDefinition* Def : AllItemDefinitions)
	{
		if (Def && !Def->ItemID.IsNone())
		{
			ItemDefMap.Add(Def->ItemID, Def);
		}
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started,   InteractionComponent, &UInteractionComponent::StartInteract);
			EIC->BindAction(IA_Interact, ETriggerEvent::Completed, InteractionComponent, &UInteractionComponent::StopInteract);
		}

		if (IA_ToggleInventory)
		{
			EIC->BindAction(IA_ToggleInventory, ETriggerEvent::Started, this, &ABaseCharacter::ToggleInventory);
		}

		if (IA_ToggleHealthUI)
		{
			EIC->BindAction(IA_ToggleHealthUI, ETriggerEvent::Started, this, &ABaseCharacter::ToggleHealthUI);
		}

		// Hotbar slot selection (keys 1-6)
		if (IA_HotbarSlot1) EIC->BindAction(IA_HotbarSlot1, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot1);
		if (IA_HotbarSlot2) EIC->BindAction(IA_HotbarSlot2, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot2);
		if (IA_HotbarSlot3) EIC->BindAction(IA_HotbarSlot3, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot3);
		if (IA_HotbarSlot4) EIC->BindAction(IA_HotbarSlot4, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot4);
		if (IA_HotbarSlot5) EIC->BindAction(IA_HotbarSlot5, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot5);
		if (IA_HotbarSlot6) EIC->BindAction(IA_HotbarSlot6, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot6);

		// Hotbar scroll (mouse wheel)
		if (IA_HotbarScrollUp) EIC->BindAction(IA_HotbarScrollUp, ETriggerEvent::Started, this, &ABaseCharacter::HotbarScrollUp);
		if (IA_HotbarScrollDown) EIC->BindAction(IA_HotbarScrollDown, ETriggerEvent::Started, this, &ABaseCharacter::HotbarScrollDown);
	}
}

void ABaseCharacter::UseItem_Implementation(int32 SlotIndex, UInventoryComponent* FromInventory)
{
	if (!FromInventory) return;

	FInventorySlot Slot = FromInventory->GetSlot(SlotIndex);
	if (Slot.IsEmpty() || !Slot.ItemDef) return;
	if (Slot.ItemDef->ItemCategory != EItemCategory::Consumable) return;

	HealthComponent->RestoreHealth(EBodyPart::Body, Slot.ItemDef->HealthRestoreAmount);
	FromInventory->RemoveItem(SlotIndex, 1);

	const FBodyPartHealth BodyPart = HealthComponent->GetBodyPart(EBodyPart::Body);
	UE_LOG(LogTemp, Log, TEXT("Used %s — Body Health: %.0f / %.0f"),
		*Slot.ItemDef->DisplayName.ToString(), BodyPart.CurrentHealth, BodyPart.MaxHealth);
}

void ABaseCharacter::EquipItem_Implementation(int32 SlotIndex, UInventoryComponent* FromInventory)
{
	if (!FromInventory) return;

	FInventorySlot Slot = FromInventory->GetSlot(SlotIndex);
	if (Slot.IsEmpty() || !Slot.ItemDef) return;
	if (Slot.ItemDef->ItemCategory != EItemCategory::Weapon) return;
	if (!Slot.ItemDef->WeaponActorClass) return;

	// Unequip any currently equipped weapon first
	UnequipWeapon();

	// Spawn the weapon actor and attach it to the character's hand socket
	FActorSpawnParameters Params;
	Params.Owner = this;
	AWeaponBase* Weapon = GetWorld()->SpawnActor<AWeaponBase>(
		Slot.ItemDef->WeaponActorClass, FTransform::Identity, Params);

	if (Weapon)
	{
		Weapon->SourceItemDef = Slot.ItemDef;
		Weapon->AttachToComponent(GetMesh(),
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			WeaponSocketName);
		EquippedWeapon = Weapon;

		UE_LOG(LogTemp, Log, TEXT("Equipped: %s"), *Slot.ItemDef->DisplayName.ToString());
	}
}

void ABaseCharacter::UnequipWeapon_Implementation()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}
}

void ABaseCharacter::OnBodyPartDamaged(EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken)
{
	// Recalculate movement speed whenever a leg takes damage.
	if (Part == EBodyPart::LeftLeg || Part == EBodyPart::RightLeg)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * HealthComponent->GetMovementSpeedMultiplier();
	}

	// Broadcast death if a vital part (Head or Body) just reached 0.
	if (bJustBroken && (Part == EBodyPart::Head || Part == EBodyPart::Body))
	{
		HealthComponent->OnDeath.Broadcast();
	}
}

UItemDefinition* ABaseCharacter::FindItemDefByID(FName ItemID) const
{
	UItemDefinition* const* Found = ItemDefMap.Find(ItemID);
	return Found ? *Found : nullptr;
}

void ABaseCharacter::SaveGame_Implementation()
{
	UTwoDSurvivalSaveGame* SaveObj = Cast<UTwoDSurvivalSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTwoDSurvivalSaveGame::StaticClass()));
	if (!SaveObj) return;

	// Inventory
	SaveObj->BaseSlotCount = InventoryComponent->BaseSlotCount;
	SaveObj->InventorySlots.Reserve(InventoryComponent->Slots.Num());
	for (const FInventorySlot& Slot : InventoryComponent->Slots)
	{
		FSavedInventorySlot Saved;
		Saved.ItemID = Slot.ItemDef ? Slot.ItemDef->ItemID : NAME_None;
		Saved.Quantity = Slot.Quantity;
		SaveObj->InventorySlots.Add(Saved);
	}

	// Hotbar
	SaveObj->ActiveHotbarSlot = HotbarComponent->ActiveSlotIndex;
	SaveObj->HotbarSlots.Reserve(HotbarComponent->HotbarSlots.Num());
	for (UItemDefinition* HotbarItem : HotbarComponent->HotbarSlots)
	{
		FSavedHotbarSlot Saved;
		Saved.ItemID = HotbarItem ? HotbarItem->ItemID : NAME_None;
		SaveObj->HotbarSlots.Add(Saved);
	}

	// Equipped weapon
	SaveObj->EquippedWeaponItemID = EquippedWeapon && EquippedWeapon->SourceItemDef
		? EquippedWeapon->SourceItemDef->ItemID : NAME_None;

	// Health — save all 6 body parts
	const TArray<EBodyPart> AllParts = {
		EBodyPart::Head, EBodyPart::Body,
		EBodyPart::LeftArm, EBodyPart::RightArm,
		EBodyPart::LeftLeg, EBodyPart::RightLeg
	};
	for (EBodyPart Part : AllParts)
	{
		FBodyPartHealth BPH = HealthComponent->GetBodyPart(Part);
		FSavedBodyPartHealth Saved;
		Saved.Part = Part;
		Saved.CurrentHealth = BPH.CurrentHealth;
		Saved.MaxHealth = BPH.MaxHealth;
		SaveObj->BodyPartHealthValues.Add(Saved);
	}

	// Position
	SaveObj->PlayerLocation = GetActorLocation();
	SaveObj->PlayerRotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveObj, SaveSlotName, SaveUserIndex);
	UE_LOG(LogTemp, Log, TEXT("Game saved to slot: %s"), *SaveSlotName);
}

void ABaseCharacter::LoadGame_Implementation()
{
	UTwoDSurvivalSaveGame* SaveObj = Cast<UTwoDSurvivalSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	if (!SaveObj)
	{
		UE_LOG(LogTemp, Warning, TEXT("No save found in slot: %s"), *SaveSlotName);
		return;
	}

	// Unequip current weapon
	UnequipWeapon();

	// Restore inventory — reset to base slot count, then rebuild
	InventoryComponent->Slots.Empty();
	InventoryComponent->Slots.SetNum(SaveObj->BaseSlotCount);
	InventoryComponent->SlotCount = SaveObj->BaseSlotCount;
	InventoryComponent->BaseSlotCount = SaveObj->BaseSlotCount;

	// Calculate total bonus slots needed from saved items
	int32 TotalBonusSlots = 0;
	for (const FSavedInventorySlot& Saved : SaveObj->InventorySlots)
	{
		if (!Saved.ItemID.IsNone())
		{
			UItemDefinition* Def = FindItemDefByID(Saved.ItemID);
			if (Def && Def->BonusSlots > 0)
			{
				TotalBonusSlots += Def->BonusSlots;
			}
		}
	}

	// Expand to full size if backpack items were present
	if (TotalBonusSlots > 0)
	{
		InventoryComponent->ExpandSlots(TotalBonusSlots);
	}

	// Fill slots from saved data
	for (int32 i = 0; i < SaveObj->InventorySlots.Num() && i < InventoryComponent->Slots.Num(); ++i)
	{
		const FSavedInventorySlot& Saved = SaveObj->InventorySlots[i];
		if (!Saved.ItemID.IsNone())
		{
			UItemDefinition* Def = FindItemDefByID(Saved.ItemID);
			if (Def)
			{
				InventoryComponent->Slots[i].ItemDef = Def;
				InventoryComponent->Slots[i].Quantity = Saved.Quantity;
			}
		}
	}
	InventoryComponent->OnInventoryChanged.Broadcast();

	// Restore hotbar
	for (int32 i = 0; i < SaveObj->HotbarSlots.Num() && i < HotbarComponent->HotbarSlots.Num(); ++i)
	{
		const FSavedHotbarSlot& Saved = SaveObj->HotbarSlots[i];
		HotbarComponent->HotbarSlots[i] = Saved.ItemID.IsNone() ? nullptr : FindItemDefByID(Saved.ItemID);
	}
	HotbarComponent->ActiveSlotIndex = SaveObj->ActiveHotbarSlot;
	HotbarComponent->OnHotbarChanged.Broadcast();

	// Restore health
	for (const FSavedBodyPartHealth& Saved : SaveObj->BodyPartHealthValues)
	{
		HealthComponent->SetBodyPartHealth(Saved.Part, Saved.CurrentHealth);
	}

	// Restore position
	SetActorLocation(SaveObj->PlayerLocation);
	SetActorRotation(SaveObj->PlayerRotation);

	// Re-equip weapon if one was equipped
	if (!SaveObj->EquippedWeaponItemID.IsNone())
	{
		UItemDefinition* WeaponDef = FindItemDefByID(SaveObj->EquippedWeaponItemID);
		if (WeaponDef)
		{
			// Find this weapon in inventory and equip it
			for (int32 i = 0; i < InventoryComponent->Slots.Num(); ++i)
			{
				if (InventoryComponent->Slots[i].ItemDef == WeaponDef)
				{
					EquipItem(i, InventoryComponent);
					break;
				}
			}
		}
	}

	// Recalculate movement speed in case legs were damaged
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * HealthComponent->GetMovementSpeedMultiplier();

	UE_LOG(LogTemp, Log, TEXT("Game loaded from slot: %s"), *SaveSlotName);
}

void ABaseCharacter::SelectHotbarSlot1() { HotbarComponent->SelectSlot(0); }
void ABaseCharacter::SelectHotbarSlot2() { HotbarComponent->SelectSlot(1); }
void ABaseCharacter::SelectHotbarSlot3() { HotbarComponent->SelectSlot(2); }
void ABaseCharacter::SelectHotbarSlot4() { HotbarComponent->SelectSlot(3); }
void ABaseCharacter::SelectHotbarSlot5() { HotbarComponent->SelectSlot(4); }
void ABaseCharacter::SelectHotbarSlot6() { HotbarComponent->SelectSlot(5); }
void ABaseCharacter::HotbarScrollUp() { HotbarComponent->CycleSlot(-1); }
void ABaseCharacter::HotbarScrollDown() { HotbarComponent->CycleSlot(1); }

void ABaseCharacter::MoveRight(float Value)
{
	if (bMovementLocked) return;

	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		// Set controller rotation to face movement direction — the AnimBP reads from this
		const float TargetYaw = (Value > 0.f) ? 0.f : 180.f;
		if (Controller)
			Controller->SetControlRotation(FRotator(0.f, TargetYaw, 0.f));
		// Move in the explicit world direction (not relative to character facing)
		const FVector MoveDirection = (Value > 0.f) ? FVector(1.f, 0.f, 0.f) : FVector(-1.f, 0.f, 0.f);
		AddMovementInput(MoveDirection, 1.f);
	}
}
