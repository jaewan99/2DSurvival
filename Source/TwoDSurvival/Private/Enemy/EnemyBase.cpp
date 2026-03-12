// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyBase.h"
#include "Character/BaseCharacter.h"
#include "Components/SkillComponent.h"
#include "World/FlashlightActor.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"
#include "UI/EnemyHealthBarWidget.h"
#include "World/WorldItem.h"
#include "Inventory/ItemDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Health — only EBodyPart::Body is used for enemies
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	// Melee hitbox — disabled by default, positioned in front of the character
	MeleeHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("MeleeHitbox"));
	MeleeHitbox->SetupAttachment(RootComponent);
	MeleeHitbox->SetBoxExtent(FVector(55.f, 20.f, 50.f));
	MeleeHitbox->SetRelativeLocation(FVector(60.f, 0.f, 0.f));
	MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeHitbox->SetCollisionObjectType(ECC_WorldDynamic);
	MeleeHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeleeHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// World-space health bar widget above the enemy's head
	HealthBarComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComp"));
	HealthBarComp->SetupAttachment(RootComponent);
	HealthBarComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComp->SetDrawSize(FVector2D(100.f, 12.f));
	HealthBarComp->SetVisibility(false); // Hidden until first damage hit

	// Constrain movement to the X/Z plane — same as the player character
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();

	MeleeHitbox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBase::OnMeleeHitboxOverlap);
	HealthComp->OnDeath.AddDynamic(this, &AEnemyBase::OnEnemyDeath);

	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;

	// Create the health bar widget on the widget component
	if (HealthBarWidgetClass)
	{
		HealthBarComp->SetWidgetClass(HealthBarWidgetClass);
		HealthBarComp->InitWidget();
		HealthBarWidgetInstance = Cast<UEnemyHealthBarWidget>(HealthBarComp->GetWidget());
	}
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EEnemyState::Dead) return;

	CurrentSpeed = GetVelocity().Size();

	switch (CurrentState)
	{
	case EEnemyState::Idle:   TickIdle(DeltaTime);   break;
	case EEnemyState::Patrol: TickPatrol(DeltaTime); break;
	case EEnemyState::Alert:  TickAlert(DeltaTime);  break;
	case EEnemyState::Chase:  TickChase(DeltaTime);  break;
	case EEnemyState::Attack: TickAttack(DeltaTime); break;
	default: break;
	}
}

// --- Player Detection ---

ABaseCharacter* AEnemyBase::DetectPlayer() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return nullptr;

	ABaseCharacter* Player = Cast<ABaseCharacter>(PC->GetPawn());
	if (!Player) return nullptr;

	const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// Flashlight in cone doubles aggro range — enemies spot the light source.
	float EffectiveRange = AggroRange;
	if (Player->EquippedFlashlight && Player->EquippedFlashlight->IsInCone(GetActorLocation()))
	{
		EffectiveRange *= 2.f;
	}

	return Dist <= EffectiveRange ? Player : nullptr;
}

// --- Per-State Tick ---

void AEnemyBase::TickIdle(float DeltaTime)
{
	ABaseCharacter* Player = DetectPlayer();
	if (Player)
	{
		CachedPlayer = Player;
		SetEnemyState(EEnemyState::Chase);
		return;
	}

	IdleTimer += DeltaTime;
	if (IdleTimer >= IdleWaitTime)
	{
		SetEnemyState(EEnemyState::Patrol);
	}
}

void AEnemyBase::TickPatrol(float DeltaTime)
{
	ABaseCharacter* Player = DetectPlayer();
	if (Player)
	{
		CachedPlayer = Player;
		SetEnemyState(EEnemyState::Chase);
		return;
	}

	float XFromSpawn = GetActorLocation().X - SpawnLocation.X;
	if (XFromSpawn >= PatrolRange)  PatrolDir = -1.f;
	if (XFromSpawn <= -PatrolRange) PatrolDir = 1.f;

	MoveInDirection(PatrolDir, PatrolSpeed);
}

void AEnemyBase::TickAlert(float DeltaTime)
{
	// Visual detection during investigation upgrades immediately to Chase.
	if (ABaseCharacter* Player = DetectPlayer())
	{
		CachedPlayer = Player;
		SetEnemyState(EEnemyState::Chase);
		return;
	}

	const float XDist = FMath::Abs(AlertLocation.X - GetActorLocation().X);

	if (!bReachedAlertLocation)
	{
		// Walk toward the noise origin.
		if (XDist <= 50.f)
		{
			bReachedAlertLocation = true;
			GetCharacterMovement()->StopMovementImmediately();
		}
		else
		{
			const float DirX = FMath::Sign(AlertLocation.X - GetActorLocation().X);
			MoveInDirection(DirX, AlertSpeed);
		}
	}
	else
	{
		// Stand and look around — give up after AlertInvestigateTime seconds.
		AlertTimer += DeltaTime;
		if (AlertTimer >= AlertInvestigateTime)
		{
			SetEnemyState(EEnemyState::Patrol);
		}
	}
}

void AEnemyBase::HearNoise(FVector NoiseOrigin)
{
	// Already actively engaging — no need to downgrade to investigation.
	if (CurrentState == EEnemyState::Chase  ||
		CurrentState == EEnemyState::Attack ||
		CurrentState == EEnemyState::Dead)
	{
		return;
	}

	AlertLocation = NoiseOrigin;
	AlertTimer = 0.f;
	bReachedAlertLocation = false;
	SetEnemyState(EEnemyState::Alert);
}

void AEnemyBase::TickChase(float DeltaTime)
{
	if (!CachedPlayer.IsValid())
	{
		SetEnemyState(EEnemyState::Idle);
		return;
	}

	ABaseCharacter* Player = CachedPlayer.Get();

	float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
	if (Dist > AggroRange * LoseAggroMultiplier)
	{
		CachedPlayer = nullptr;
		SetEnemyState(EEnemyState::Idle);
		return;
	}

	float XDist = FMath::Abs(Player->GetActorLocation().X - GetActorLocation().X);
	if (XDist <= AttackRange && bCanAttack)
	{
		SetEnemyState(EEnemyState::Attack);
		return;
	}

	float DirX = FMath::Sign(Player->GetActorLocation().X - GetActorLocation().X);
	MoveInDirection(DirX, ChaseSpeed);
}

void AEnemyBase::TickAttack(float DeltaTime)
{
	if (!CachedPlayer.IsValid())
	{
		SetEnemyState(EEnemyState::Idle);
		return;
	}

	ABaseCharacter* Player = CachedPlayer.Get();

	// Facing is locked for AttackRotationLockDuration seconds from swing start.
	// After that window expires the enemy can face the player again before the next swing.
	if (!bRotationLocked)
	{
		float DirX = FMath::Sign(Player->GetActorLocation().X - GetActorLocation().X);
		if (!FMath::IsNearlyZero(DirX))
		{
			SetActorRotation(DirX > 0.f ? FRotator(0.f, 0.f, 0.f) : FRotator(0.f, 180.f, 0.f));
		}
	}

	float XDist = FMath::Abs(Player->GetActorLocation().X - GetActorLocation().X);

	// Player moved out of range — go back to chasing once the stand-still window ends
	if (XDist > AttackRange && !bIsAttacking && !bPostAttackMoveLocked)
	{
		SetEnemyState(EEnemyState::Chase);
		return;
	}

	// Initiate a new swing once both the cooldown and stand-still window have cleared
	if (bCanAttack && !bIsAttacking && !bPostAttackMoveLocked)
	{
		BeginMeleeAttack();
	}
}

// --- State Management ---

void AEnemyBase::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState) return;
	if (CurrentState == EEnemyState::Dead) return; // Dead is terminal — no transitions out
	CurrentState = NewState;

	switch (NewState)
	{
	case EEnemyState::Idle:
		IdleTimer = 0.f;
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		break;
	case EEnemyState::Patrol:
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		break;
	case EEnemyState::Alert:
		GetCharacterMovement()->MaxWalkSpeed = AlertSpeed;
		break;
	case EEnemyState::Chase:
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		break;
	case EEnemyState::Attack:
		// Cancel chase momentum immediately so the enemy plants before swinging
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		break;
	case EEnemyState::Dead:
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		GetWorldTimerManager().ClearTimer(SwingTimerHandle);
		GetWorldTimerManager().ClearTimer(AttackCooldownTimer);
		GetWorldTimerManager().ClearTimer(PostAttackMoveTimer);
		GetWorldTimerManager().ClearTimer(RotationLockTimer);
		break;
	}
}

void AEnemyBase::MoveInDirection(float DirX, float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	AddMovementInput(FVector(DirX, 0.f, 0.f), 1.f);
	SetActorRotation(DirX > 0.f ? FRotator(0.f, 0.f, 0.f) : FRotator(0.f, 180.f, 0.f));
}

// --- Combat ---

void AEnemyBase::BeginMeleeAttack()
{
	// Guard: don't start a new swing if hitbox is already active
	if (MeleeHitbox->GetCollisionEnabled() != ECollisionEnabled::NoCollision) return;

	bIsAttacking = true;
	bCanAttack = false;
	bPostAttackMoveLocked = true;
	bRotationLocked = true;
	HitActorsThisSwing.Empty();

	// Hitbox is NOT enabled here — AnimNotify_BeginAttack fires at the correct hit frame.
	// SwingTimerHandle acts as a safety fallback to close the hitbox if the notify is missed.

	if (AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage);
	}

	// Unlock attack input after cooldown
	GetWorldTimerManager().SetTimer(AttackCooldownTimer, this, &AEnemyBase::ResetAttackCooldown, AttackCooldown, false);

	// Unlock movement after cooldown + extra stand-still window
	GetWorldTimerManager().SetTimer(PostAttackMoveTimer, this, &AEnemyBase::OnPostAttackMoveUnlocked, AttackCooldown + PostAttackStandStillDuration, false);

	// Unlock rotation after configurable duration
	GetWorldTimerManager().SetTimer(RotationLockTimer, this, &AEnemyBase::OnRotationLockExpired, AttackRotationLockDuration, false);
}

void AEnemyBase::EnableMeleeHitbox()
{
	if (CurrentState == EEnemyState::Dead) return;
	HitActorsThisSwing.Empty();
	MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Safety fallback: auto-close after SwingWindowDuration if EndAttack notify is absent
	GetWorldTimerManager().SetTimer(SwingTimerHandle, this, &AEnemyBase::DisableMeleeHitbox, SwingWindowDuration, false);
}

void AEnemyBase::DisableMeleeHitbox()
{
	MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().ClearTimer(SwingTimerHandle);
}

void AEnemyBase::EndMeleeAttack()
{
	DisableMeleeHitbox();
}

void AEnemyBase::ResetAttackCooldown()
{
	bIsAttacking = false;
	bCanAttack = true;
}

void AEnemyBase::OnPostAttackMoveUnlocked()
{
	bPostAttackMoveLocked = false;
}

void AEnemyBase::OnRotationLockExpired()
{
	bRotationLocked = false;
}

void AEnemyBase::OnMeleeHitboxOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;
	if (HitActorsThisSwing.Contains(OtherActor)) return;

	IDamageable* Target = Cast<IDamageable>(OtherActor);
	if (!Target) return;

	HitActorsThisSwing.Add(OtherActor);
	Target->Execute_TakeMeleeDamage(OtherActor, BaseDamage, this);
}

// --- IDamageable ---

void AEnemyBase::TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource)
{
	if (CurrentState == EEnemyState::Dead) return;

	HealthComp->ApplyDamage(EBodyPart::Body, Amount);

	// ApplyDamage may have fired OnDeath synchronously — don't override Dead state
	if (CurrentState == EEnemyState::Dead) return;

	ShowHealthBar();
	UpdateHealthBar();

	// Aggro the attacker if we haven't already
	if (ABaseCharacter* Attacker = Cast<ABaseCharacter>(DamageSource))
	{
		CachedPlayer = Attacker;
		if (CurrentState != EEnemyState::Attack)
		{
			SetEnemyState(EEnemyState::Chase);
		}
	}
}

// --- Health Bar ---

void AEnemyBase::ShowHealthBar()
{
	HealthBarComp->SetVisibility(true);

	// Widget may not be ready in BeginPlay — cache lazily on first damage
	if (!HealthBarWidgetInstance)
	{
		HealthBarWidgetInstance = Cast<UEnemyHealthBarWidget>(HealthBarComp->GetWidget());
	}
}

void AEnemyBase::UpdateHealthBar()
{
	if (HealthBarWidgetInstance)
	{
		HealthBarWidgetInstance->SetHealthPercent(HealthComp->GetHealthPercent(EBodyPart::Body));
	}
}

// --- Death ---

void AEnemyBase::OnEnemyDeath()
{
	SetEnemyState(EEnemyState::Dead);

	// Disable collisions so the player can walk through the corpse
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarComp->SetVisibility(false);

	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
	}

	SpawnLoot();

	// Grant combat XP to the player who killed this enemy.
	if (ABaseCharacter* Killer = CachedPlayer.Get())
	{
		if (Killer->SkillComponent)
		{
			Killer->SkillComponent->AddXP(ESkillType::Combat, Killer->SkillComponent->CombatXPPerKill);
		}
	}

	GetWorldTimerManager().SetTimer(DeathDestroyTimer, this, &AEnemyBase::DestroyEnemy, DeathDestroyDelay, false);
}

void AEnemyBase::SpawnLoot()
{
	UWorld* World = GetWorld();
	if (!World || LootTable.Num() == 0) return;

	// Scavenging Lv2: each loot entry gets one extra roll.
	int32 ExtraRolls = 0;
	if (ABaseCharacter* Player = CachedPlayer.Get())
	{
		if (Player->SkillComponent)
			ExtraRolls = Player->SkillComponent->GetExtraLootRolls();
	}

	// Helper lambda: attempt one drop roll for an entry.
	auto TryDropEntry = [&](const FLootEntry& Entry)
	{
		if (!Entry.ItemDef) return;
		if (FMath::FRand() > Entry.DropChance) return;

		int32 Count = FMath::RandRange(Entry.MinCount, Entry.MaxCount);
		if (Count <= 0) return;

		FVector DropLoc = GetActorLocation() + FVector(FMath::RandRange(-40.f, 40.f), 0.f, 10.f);
		AWorldItem* Pickup = World->SpawnActor<AWorldItem>(AWorldItem::StaticClass(), FTransform(FRotator::ZeroRotator, DropLoc));
		if (Pickup)
		{
			Pickup->ItemDef = Entry.ItemDef;
			Pickup->Quantity = Count;
		}
	};

	for (const FLootEntry& Entry : LootTable)
	{
		TryDropEntry(Entry);

		// Scavenging Lv2: one extra independent roll per entry.
		for (int32 i = 0; i < ExtraRolls; ++i)
		{
			TryDropEntry(Entry);
		}
	}
}

void AEnemyBase::DestroyEnemy()
{
	Destroy();
}
