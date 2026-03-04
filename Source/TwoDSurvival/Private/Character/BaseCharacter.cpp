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
#include "UI/CraftingWidget.h"
#include "UI/NeedsWarningWidget.h"
#include "Crafting/CraftingComponent.h"
#include "World/TimeManager.h"
// DamageableInterface included via BaseCharacter.h

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

	// Crafting component — recipe scanning and craft execution
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));

	// Needs component — tracks Hunger, Thirst, Fatigue over time
	NeedsComponent = CreateDefaultSubobject<UNeedsComponent>(TEXT("NeedsComponent"));

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
	IA_Attack = MakeAction(TEXT("IA_Attack"));

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
	GameplayIMC->MapKey(IA_Attack, EKeys::LeftMouseButton);
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

	// Handle player death — disable input, play montage, fire BP event.
	HealthComponent->OnDeath.AddDynamic(this, &ABaseCharacter::HandlePlayerDeath);

	// Bind needs delegate — recalculates movement speed whenever any need changes
	NeedsComponent->OnNeedChanged.AddDynamic(this, &ABaseCharacter::OnNeedChanged);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Always show the mouse cursor — GameAndUI lets game input and UI input coexist
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		// Create the always-visible hotbar widget
		if (HotbarWidgetClass)
		{
			HotbarWidgetInstance = CreateWidget<UHotbarWidget>(PC, HotbarWidgetClass);
			if (HotbarWidgetInstance)
			{
				HotbarWidgetInstance->AddToViewport(0);
			}
		}

		// Create the always-visible needs warning widget (positions itself in NativeConstruct)
		if (NeedsWarningWidgetClass)
		{
			NeedsWarningInstance = CreateWidget<UNeedsWarningWidget>(PC, NeedsWarningWidgetClass);
			if (NeedsWarningInstance)
			{
				NeedsWarningInstance->AddToViewport(1);
			}
		}
	}

	// Auto-scan all item definitions via AssetRegistry
	ScanItemDefinitions();

	// Create dynamic material instances on every mesh slot so we can drive
	// the CharacterForward parameter for the backside-darkening shader.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		const int32 NumMats = SkelMesh->GetNumMaterials();
		CharacterMIDs.Reserve(NumMats);
		for (int32 i = 0; i < NumMats; i++)
		{
			CharacterMIDs.Add(SkelMesh->CreateAndSetMaterialInstanceDynamic(i));
		}
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Push the current facing direction to the backside-darkening material.
	// Only update when the forward vector actually changes (character turned around).
	const FVector CurrentForward = GetActorForwardVector();
	if (!CurrentForward.Equals(LastForwardForMaterial, 0.01f))
	{
		LastForwardForMaterial = CurrentForward;
		const FLinearColor ForwardParam(CurrentForward.X, CurrentForward.Y, CurrentForward.Z, 0.f);
		for (UMaterialInstanceDynamic* MID : CharacterMIDs)
		{
			if (MID) MID->SetVectorParameterValue(TEXT("CharacterForward"), ForwardParam);
		}
	}

	// While sleeping, forcibly halt movement every tick.
	// Blueprint movement input bypasses bMovementLocked, so we zero velocity here as a guarantee.
	if (NeedsComponent && NeedsComponent->bIsSleeping)
	{
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}

	// Double the drain rate when running (~100 cm/s) or attacking
	const bool bIsMovingFast = GetVelocity().SizeSquared() > 10000.f;
	NeedsComponent->SetActiveMovement(bIsMovingFast || bIsAttacking);
}

void ABaseCharacter::Jump()
{
	if (bIsAttacking) return;
	Super::Jump();
}

void ABaseCharacter::Crouch(bool bClientSimulation)
{
	if (bIsAttacking) return;
	Super::Crouch(bClientSimulation);
}

void ABaseCharacter::UnCrouch(bool bClientSimulation)
{
	if (bIsAttacking) return;
	Super::UnCrouch(bClientSimulation);
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
			EIC->BindAction(IA_Interact, ETriggerEvent::Started,   this, &ABaseCharacter::OnInteractStarted);
			EIC->BindAction(IA_Interact, ETriggerEvent::Completed, this, &ABaseCharacter::OnInteractCompleted);
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

void ABaseCharacter::ShowUICursor()
{
	++UIOpenCount;
}

void ABaseCharacter::HideUICursor()
{
	UIOpenCount = FMath::Max(0, UIOpenCount - 1);
}

void ABaseCharacter::RegisterTooltip(UUserWidget* Tooltip)
{
	ActiveItemTooltip = Tooltip;
}

void ABaseCharacter::ClearTooltip()
{
	if (ActiveItemTooltip)
	{
		ActiveItemTooltip->RemoveFromParent();
		ActiveItemTooltip = nullptr;
	}
}

void ABaseCharacter::ToggleHealthUI_Implementation()
{
	if (HealthHUDInstance)
	{
		HealthHUDInstance->RemoveFromParent();
		HealthHUDInstance = nullptr;
		HideUICursor();
	}
	else if (HealthHUDWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;

		HealthHUDInstance = CreateWidget<UHealthHUDWidget>(PC, HealthHUDWidgetClass);
		if (HealthHUDInstance)
		{
			HealthHUDInstance->AddToViewport(10);
			const FVector2D InitialPos(50.f, 50.f);
			HealthHUDInstance->SetPositionInViewport(InitialPos, false);
			HealthHUDInstance->InitDragPosition(InitialPos);
			ShowUICursor();
		}
	}
}

void ABaseCharacter::ToggleCrafting_Implementation()
{
	if (CraftingWidgetInstance)
	{
		CraftingWidgetInstance->RemoveFromParent();
		CraftingWidgetInstance = nullptr;
		HideUICursor();
	}
	else if (CraftingWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;

		CraftingWidgetInstance = CreateWidget<UCraftingWidget>(PC, CraftingWidgetClass);
		if (CraftingWidgetInstance)
		{
			CraftingWidgetInstance->AddToViewport(5);
			ShowUICursor();
		}
	}
}

void ABaseCharacter::CloseCrafting_Implementation()
{
	if (!CraftingWidgetInstance) return;

	CraftingWidgetInstance->RemoveFromParent();
	CraftingWidgetInstance = nullptr;
	HideUICursor();
}

void ABaseCharacter::StartSleeping_Implementation()
{
	bMovementLocked = true;

	// Set bIsSleeping BEFORE SetMovementMode — MOVE_None triggers an overlap recalc that
	// fires OnBoxEndOverlap on the bed's InteractionBox. The guard there checks bIsSleeping
	// to avoid immediately waking the player due to that physics artifact.
	if (NeedsComponent)
	{
		NeedsComponent->SleepStartFatigue = NeedsComponent->GetNeedValue(ENeedType::Fatigue);
		NeedsComponent->bIsSleeping = true;
		NeedsComponent->SleepTimeScaleMultiplier = SleepTimeScale;
	}

	// Block all AddMovementInput calls (C++ and Blueprint paths)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		PC->SetIgnoreMoveInput(true);

	// Disable movement mode — prevents root motion from driving movement
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	// Speed up the world clock so morning arrives faster
	if (ATimeManager* TM = Cast<ATimeManager>(UGameplayStatics::GetActorOfClass(this, ATimeManager::StaticClass())))
		TM->SetTimeScale(SleepTimeScale);
}

void ABaseCharacter::StopSleeping_Implementation()
{
	bMovementLocked = false;

	// Restore movement input and physics
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		PC->SetIgnoreMoveInput(false);

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	if (NeedsComponent)
	{
		NeedsComponent->bIsSleeping = false;
		NeedsComponent->SleepTimeScaleMultiplier = 1.f;
	}

	// Restore normal time flow
	if (ATimeManager* TM = Cast<ATimeManager>(UGameplayStatics::GetActorOfClass(this, ATimeManager::StaticClass())))
	{
		TM->SetTimeScale(1.f);
	}

	RecalculateMovementSpeed();
}

void ABaseCharacter::UseItem_Implementation(int32 SlotIndex, UInventoryComponent* FromInventory)
{
	if (!FromInventory) return;

	FInventorySlot Slot = FromInventory->GetSlot(SlotIndex);
	if (Slot.IsEmpty() || !Slot.ItemDef) return;
	if (Slot.ItemDef->ItemCategory != EItemCategory::Consumable) return;

	HealthComponent->RestoreHealth(EBodyPart::Body, Slot.ItemDef->HealthRestoreAmount);

	if (NeedsComponent)
	{
		if (Slot.ItemDef->HungerRestore  > 0.f) NeedsComponent->RestoreNeed(ENeedType::Hunger,  Slot.ItemDef->HungerRestore);
		if (Slot.ItemDef->ThirstRestore  > 0.f) NeedsComponent->RestoreNeed(ENeedType::Thirst,  Slot.ItemDef->ThirstRestore);
		if (Slot.ItemDef->FatigueRestore > 0.f) NeedsComponent->RestoreNeed(ENeedType::Fatigue, Slot.ItemDef->FatigueRestore);
	}

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

void ABaseCharacter::RecalculateMovementSpeed()
{
	// Movement mode is MOVE_None while sleeping — MaxWalkSpeed is irrelevant, skip
	if (NeedsComponent && NeedsComponent->bIsSleeping) return;

	const float HealthMult = HealthComponent ? HealthComponent->GetMovementSpeedMultiplier() : 1.f;
	const float NeedsMult  = NeedsComponent  ? NeedsComponent->GetSpeedMultiplier()          : 1.f;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * HealthMult * NeedsMult;
}

void ABaseCharacter::OnNeedChanged(ENeedType NeedType, float CurrentValue, float MaxValue, bool bIsWarning)
{
	RecalculateMovementSpeed();
}

void ABaseCharacter::OnBodyPartDamaged(EBodyPart Part, float CurrentHealth, float MaxHealth, bool bJustBroken)
{
	// Recalculate movement speed whenever a leg takes damage.
	if (Part == EBodyPart::LeftLeg || Part == EBodyPart::RightLeg)
	{
		RecalculateMovementSpeed();
	}
	// Note: OnDeath is broadcast directly from UHealthComponent::ApplyDamage when Head/Body breaks.
	// No need to broadcast it here — doing so would cause HandlePlayerDeath to fire twice.
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

	// Needs
	SaveObj->SavedHunger  = NeedsComponent->GetNeedValue(ENeedType::Hunger);
	SaveObj->SavedThirst  = NeedsComponent->GetNeedValue(ENeedType::Thirst);
	SaveObj->SavedFatigue = NeedsComponent->GetNeedValue(ENeedType::Fatigue);

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

	// Restore needs
	NeedsComponent->SetNeedValue(ENeedType::Hunger,  SaveObj->SavedHunger);
	NeedsComponent->SetNeedValue(ENeedType::Thirst,  SaveObj->SavedThirst);
	NeedsComponent->SetNeedValue(ENeedType::Fatigue, SaveObj->SavedFatigue);

	// Recalculate movement speed accounting for leg damage and needs
	RecalculateMovementSpeed();

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

void ABaseCharacter::TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource)
{
	if (!HealthComponent || HealthComponent->IsDead()) return;

	// Apply damage to the Body by default. Enemies that want per-part hits
	// can call HealthComponent->ApplyDamage directly with a specific EBodyPart.
	HealthComponent->ApplyDamage(EBodyPart::Body, Amount);

	UE_LOG(LogTemp, Log, TEXT("BaseCharacter took %.1f melee damage from %s"),
		Amount, DamageSource ? *DamageSource->GetName() : TEXT("Unknown"));
}

void ABaseCharacter::HandlePlayerDeath()
{
	// Prevent repeated calls (OnDeath could fire more than once in edge cases)
	if (!HealthComponent || !HealthComponent->IsDead()) return;

	// Lock all movement input
	bMovementLocked = true;

	// Play the death animation if assigned
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// Disable player input entirely (movement, hotbar, attack, etc.)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// Notify Blueprint — override OnPlayerDied in BP_BaseCharacter to show a respawn screen.
	OnPlayerDied();

	UE_LOG(LogTemp, Log, TEXT("BaseCharacter: Player died."));
}

void ABaseCharacter::OnAttackPressed()
{
	if (bIsAttacking) return;

	// Don't interrupt an in-progress traversal action (vault, climb, etc.)
	if (bIsTraversing) return;

	// Don't attack while any UI panel is open (inventory, crafting, health HUD, etc.)
	if (UIOpenCount > 0) return;

	bIsAttacking = true;

	UAnimMontage* MontageToPlay = nullptr;

	if (EquippedWeapon)
	{
		// Armed attack — play the montage only.
		// AnimNotify_BeginAttack in the montage calls EquippedWeapon->BeginAttack() at the
		// precise swing frame; AnimNotify_EndAttack closes the hitbox at the recovery frame.
		MontageToPlay = EquippedWeapon->AttackMontage;
	}
	else
	{
		// Unarmed — play punch montage, then do a delayed sphere sweep for hit detection.
		MontageToPlay = UnarmedAttackMontage;
		GetWorldTimerManager().SetTimer(
			UnarmedHitTimer, this, &ABaseCharacter::PerformUnarmedHit, UnarmedHitDelay, false);
	}

	if (MontageToPlay)
	{
		const float MontageLength = PlayAnimMontage(MontageToPlay);

		if (MontageLength > 0.f)
		{
			// Montage started successfully — reset bIsAttacking (and IK) exactly when it ends.
			// Do NOT start the fallback timer; it would fire mid-animation for long montages
			// (e.g. hammer swing > 0.7s) and re-enable IK before the animation finishes.
			if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
			{
				AnimInst->OnMontageEnded.RemoveDynamic(this, &ABaseCharacter::OnAttackMontageEnded);
				AnimInst->OnMontageEnded.AddDynamic(this, &ABaseCharacter::OnAttackMontageEnded);
			}
			return;
		}
	}

	// Fallback: montage not assigned or failed to play — reset via timer so the player
	// is never permanently locked out of attacking.
	GetWorldTimerManager().SetTimer(
		AttackCooldownTimer, this, &ABaseCharacter::ResetAttack, AttackCooldownDuration, false);
}

void ABaseCharacter::ResetAttack()
{
	// Called by the safety-fallback timer (no montage assigned, or montage ended normally first).
	bIsAttacking = false;

	// Clean up the montage-end delegate in case the montage already ended and fired it
	if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInst->OnMontageEnded.RemoveDynamic(this, &ABaseCharacter::OnAttackMontageEnded);
	}
}

void ABaseCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Only react to our own attack montages
	const bool bWasWeaponMontage  = EquippedWeapon && Montage == EquippedWeapon->AttackMontage;
	const bool bWasUnarmedMontage = Montage == UnarmedAttackMontage;
	if (!bWasWeaponMontage && !bWasUnarmedMontage) return;

	bIsAttacking = false;

	// Cancel the safety-fallback timer — montage ended naturally, no need for it
	GetWorldTimerManager().ClearTimer(AttackCooldownTimer);

	// Remove self from the delegate — each attack re-adds it
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->OnMontageEnded.RemoveDynamic(this, &ABaseCharacter::OnAttackMontageEnded);
	}
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

		const float NeedsMult = NeedsComponent ? NeedsComponent->GetDamageMultiplier() : 1.f;
		const float FinalDamage = UnarmedDamage * NeedsMult;
		IDamageable::Execute_TakeMeleeDamage(HitActor, FinalDamage, this);
		UE_LOG(LogTemp, Log, TEXT("Punch: Hit %s for %.1f damage (needs mult=%.2f)"), *HitActor->GetName(), FinalDamage, NeedsMult);
		break; // Punch hits one target at a time
	}
}

void ABaseCharacter::OnInteractStarted()
{
	if (bIsAttacking) return;
	InteractionComponent->StartInteract();
}

void ABaseCharacter::OnInteractCompleted()
{
	// Always forward release so a hold-interaction that was started before an attack
	// doesn't get stuck with its timer running.
	InteractionComponent->StopInteract();
}

void ABaseCharacter::MoveRight(float Value)
{
	if (bMovementLocked || bIsAttacking) return;

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
