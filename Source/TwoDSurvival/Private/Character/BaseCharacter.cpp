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
#include "Weapon/WeaponBase.h"

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
