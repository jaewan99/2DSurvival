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
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/HotbarComponent.h"
#include "Weapon/WeaponBase.h"
#include "Save/TwoDSurvivalSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/HealthHUDWidget.h"
#include "UI/HotbarWidget.h"
#include "Combat/DamageableInterface.h"

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

void ABaseCharacter::CreateInputActions()
{
	// Helper lambda to create a boolean input action
	auto MakeAction = [this](const TCHAR* Name) -> UInputAction*
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		return Action;
	};

	IA_HotbarSlot1 = MakeAction(TEXT("IA_HotbarSlot1"));
	IA_HotbarSlot2 = MakeAction(TEXT("IA_HotbarSlot2"));
	IA_HotbarSlot3 = MakeAction(TEXT("IA_HotbarSlot3"));
	IA_HotbarSlot4 = MakeAction(TEXT("IA_HotbarSlot4"));
	IA_HotbarSlot5 = MakeAction(TEXT("IA_HotbarSlot5"));
	IA_HotbarSlot6 = MakeAction(TEXT("IA_HotbarSlot6"));
	IA_HotbarScrollUp = MakeAction(TEXT("IA_HotbarScrollUp"));
	IA_HotbarScrollDown = MakeAction(TEXT("IA_HotbarScrollDown"));
	IA_SaveGameAction = MakeAction(TEXT("IA_SaveGame"));
	IA_LoadGameAction = MakeAction(TEXT("IA_LoadGame"));
	IA_Attack         = MakeAction(TEXT("IA_Attack"));

	// Create mapping context and bind keys
	GameplayIMC = NewObject<UInputMappingContext>(this, TEXT("GameplayIMC"));

	GameplayIMC->MapKey(IA_HotbarSlot1, EKeys::One);
	GameplayIMC->MapKey(IA_HotbarSlot2, EKeys::Two);
	GameplayIMC->MapKey(IA_HotbarSlot3, EKeys::Three);
	GameplayIMC->MapKey(IA_HotbarSlot4, EKeys::Four);
	GameplayIMC->MapKey(IA_HotbarSlot5, EKeys::Five);
	GameplayIMC->MapKey(IA_HotbarSlot6, EKeys::Six);
	GameplayIMC->MapKey(IA_HotbarScrollUp, EKeys::MouseScrollUp);
	GameplayIMC->MapKey(IA_HotbarScrollDown, EKeys::MouseScrollDown);
	GameplayIMC->MapKey(IA_SaveGameAction, EKeys::F5);
	GameplayIMC->MapKey(IA_LoadGameAction, EKeys::F9);
	GameplayIMC->MapKey(IA_Attack,         EKeys::LeftMouseButton);
}

void ABaseCharacter::ScanItemDefinitions()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UItemDefinition::StaticClass()->GetClassPathName(), AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		UItemDefinition* Def = Cast<UItemDefinition>(AssetData.GetAsset());
		if (Def && !Def->ItemID.IsNone())
		{
			ItemDefMap.Add(Def->ItemID, Def);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Auto-scanned %d item definitions."), ItemDefMap.Num());
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Store the default walk speed so leg damage can scale it relative to this base.
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// React to body part damage — adjust movement speed and trigger death.
	HealthComponent->OnBodyPartDamaged.AddDynamic(this, &ABaseCharacter::OnBodyPartDamaged);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Create the always-visible hotbar widget
		if (HotbarWidgetClass)
		{
			HotbarWidgetInstance = CreateWidget<UHotbarWidget>(PC, HotbarWidgetClass);
			if (HotbarWidgetInstance)
			{
				HotbarWidgetInstance->AddToViewport(0);
			}
		}
	}

	// Auto-scan all item definitions via AssetRegistry
	ScanItemDefinitions();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Create programmatic input actions here — guaranteed before bindings regardless of BeginPlay ordering
	CreateInputActions();

	// Add the programmatic IMC to the subsystem
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(GameplayIMC, 1);
		}
	}

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

		// Hotbar slot selection (keys 1-6) — programmatic actions, always valid
		EIC->BindAction(IA_HotbarSlot1, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot1);
		EIC->BindAction(IA_HotbarSlot2, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot2);
		EIC->BindAction(IA_HotbarSlot3, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot3);
		EIC->BindAction(IA_HotbarSlot4, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot4);
		EIC->BindAction(IA_HotbarSlot5, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot5);
		EIC->BindAction(IA_HotbarSlot6, ETriggerEvent::Started, this, &ABaseCharacter::SelectHotbarSlot6);

		// Hotbar scroll (mouse wheel)
		EIC->BindAction(IA_HotbarScrollUp, ETriggerEvent::Started, this, &ABaseCharacter::HotbarScrollUp);
		EIC->BindAction(IA_HotbarScrollDown, ETriggerEvent::Started, this, &ABaseCharacter::HotbarScrollDown);

		// Save/Load (F5/F9)
		EIC->BindAction(IA_SaveGameAction, ETriggerEvent::Started, this, &ABaseCharacter::OnSaveGamePressed);
		EIC->BindAction(IA_LoadGameAction, ETriggerEvent::Started, this, &ABaseCharacter::OnLoadGamePressed);

		// Attack (LMB)
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ABaseCharacter::OnAttackPressed);
	}
}

void ABaseCharacter::ToggleHealthUI_Implementation()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (HealthHUDInstance)
	{
		HealthHUDInstance->RemoveFromParent();
		HealthHUDInstance = nullptr;
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else if (HealthHUDWidgetClass)
	{
		HealthHUDInstance = CreateWidget<UHealthHUDWidget>(PC, HealthHUDWidgetClass);
		if (HealthHUDInstance)
		{
			HealthHUDInstance->AddToViewport(10);
			// Set initial position — fixes the "fill screen" issue on first spawn
			const FVector2D InitialPos(50.f, 50.f);
			HealthHUDInstance->SetPositionInViewport(InitialPos, false);
			// Tell the widget where it is so the drag system starts from the correct position
			HealthHUDInstance->InitDragPosition(InitialPos);
			PC->bShowMouseCursor = true;
			// GameAndUI lets the first click register as drag immediately — no "click to focus" needed
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
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
void ABaseCharacter::OnSaveGamePressed() { SaveGame(); }
void ABaseCharacter::OnLoadGamePressed() { LoadGame(); }

void ABaseCharacter::OnAttackPressed()
{
	if (bIsAttacking) return;

	bIsAttacking = true;

	if (EquippedWeapon)
	{
		// Armed attack — play the weapon's own montage and enable its hitbox
		if (EquippedWeapon->AttackMontage)
		{
			PlayAnimMontage(EquippedWeapon->AttackMontage);
		}
		const float DamageMultiplier = HealthComponent->GetDamageMultiplier();
		EquippedWeapon->BeginAttack(DamageMultiplier);
	}
	else
	{
		// Unarmed — play punch montage, then do a delayed sphere sweep for hit detection
		if (UnarmedAttackMontage)
		{
			PlayAnimMontage(UnarmedAttackMontage);
		}
		GetWorldTimerManager().SetTimer(
			UnarmedHitTimer, this, &ABaseCharacter::PerformUnarmedHit, UnarmedHitDelay, false);
	}

	// Reset the attack lock after the full cooldown
	GetWorldTimerManager().SetTimer(
		AttackCooldownTimer, this, &ABaseCharacter::ResetAttack, AttackCooldownDuration, false);
}

void ABaseCharacter::ResetAttack()
{
	bIsAttacking = false;
}

void ABaseCharacter::PerformUnarmedHit()
{
	const FVector Start   = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const FVector End     = Start + Forward * UnarmedHitRange;

	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(UnarmedHitRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn, Sphere, Params);

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;
		if (!HitActor->GetClass()->ImplementsInterface(UDamageable::StaticClass())) continue;

		IDamageable::Execute_TakeMeleeDamage(HitActor, UnarmedDamage, this);
		UE_LOG(LogTemp, Log, TEXT("Punch: Hit %s for %.1f damage"), *HitActor->GetName(), UnarmedDamage);
		break; // Punch hits one target at a time
	}
}

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
