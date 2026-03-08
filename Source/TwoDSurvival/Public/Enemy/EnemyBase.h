// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/DamageableInterface.h"
#include "Enemy/EnemyTypes.h"
#include "EnemyBase.generated.h"

class UHealthComponent;
class UBoxComponent;
class UWidgetComponent;
class UAnimMontage;
class ABaseCharacter;
class UEnemyHealthBarWidget;

/**
 * Base class for all zombie-like melee enemies.
 * Uses a C++ state machine (Idle → Patrol → Chase → Attack → Dead).
 * Implements IDamageable so player weapons can hit it.
 * Subclass in Blueprint (BP_EnemyBase) to assign mesh, AnimBP, loot table, and montages.
 */
UCLASS()
class TWODSURVIVAL_API AEnemyBase : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	AEnemyBase();

	// --- Components ---

	// Per-body-part health (only EBodyPart::Body is used for enemies).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UHealthComponent> HealthComp;

	// Melee attack hitbox. Disabled by default — enabled during swing window.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UBoxComponent> MeleeHitbox;

	// World-space widget showing health bar above the enemy's head.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarComp;

	// --- AI Config (EditDefaultsOnly — tune in BP_EnemyBase) ---

	// Radius within which the enemy detects and chases the player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AggroRange = 800.f;

	// X-axis distance at which the enemy starts attacking.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 90.f;

	// Max distance the enemy wanders from its spawn point while patrolling.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float PatrolRange = 400.f;

	// Aggro is lost when player distance exceeds AggroRange * LoseAggroMultiplier.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float LoseAggroMultiplier = 1.5f;

	// How long the enemy stands idle before switching to patrol.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float IdleWaitTime = 2.f;

	// Seconds before the actor is destroyed after death.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float DeathDestroyDelay = 3.f;

	// --- Combat Config ---

	// Damage dealt per melee hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float BaseDamage = 20.f;

	// Seconds between attacks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float AttackCooldown = 1.5f;

	// How long the melee hitbox stays active per swing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float SwingWindowDuration = 0.4f;

	// Extra stand-still time AFTER AttackCooldown before the enemy can move/re-attack.
	// Prevents sliding when the walk animation kicks in before the attack pose settles.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float PostAttackStandStillDuration = 0.3f;

	// How long the enemy's facing direction is locked after a swing starts.
	// Set to 0 to always track the player between attacks; increase to give the player
	// a longer window to jump over the enemy mid-swing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float AttackRotationLockDuration = 0.5f;

	// --- Movement Config ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float ChaseSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float PatrolSpeed = 150.f;

	// --- Assign in BP_EnemyBase ---

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// Items this enemy can drop on death. Configure in BP_EnemyBase Details panel.
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<FLootEntry> LootTable;

	// Assign WBP_EnemyHealthBar here in BP_EnemyBase.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// --- IDamageable ---

	virtual void TakeMeleeDamage_Implementation(float Amount, AActor* DamageSource) override;

	// Called by AnimNotify_BeginAttack — enables the melee hitbox at the correct animation frame.
	void EnableMeleeHitbox();

	// Called by AnimNotify_EndAttack — disables the melee hitbox early for frame-accurate close.
	void DisableMeleeHitbox();

	// --- AnimBP-readable state ---

	// Current AI state — read in ABP_Enemy to drive locomotion.
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EEnemyState CurrentState = EEnemyState::Idle;

	// Speed magnitude — read by AnimBP blend space.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentSpeed = 0.f;

	// True while a swing is in progress — read by AnimBP.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	// World position at BeginPlay — patrol wanders around this point.
	FVector SpawnLocation;

	// +1 = patrolling right, -1 = patrolling left.
	float PatrolDir = 1.f;

	// Accumulated time spent in Idle state.
	float IdleTimer = 0.f;

	// Weak ref to the player — set on aggro, cleared on lose-aggro.
	TWeakObjectPtr<ABaseCharacter> CachedPlayer;

	// False during attack cooldown.
	bool bCanAttack = true;

	// Actors already hit this swing — prevents multi-hit per swing.
	TSet<AActor*> HitActorsThisSwing;

	// True from swing start until AttackCooldown + PostAttackStandStillDuration elapses.
	// Blocks state transitions and new attacks during that window.
	bool bPostAttackMoveLocked = false;

	// True from swing start until AttackRotationLockDuration elapses.
	// Freezes facing direction so the player can jump over mid-swing.
	bool bRotationLocked = false;

	FTimerHandle SwingTimerHandle;
	FTimerHandle AttackCooldownTimer;
	FTimerHandle PostAttackMoveTimer;
	FTimerHandle RotationLockTimer;
	FTimerHandle DeathDestroyTimer;

	// Returns the player if within AggroRange, nullptr otherwise.
	ABaseCharacter* DetectPlayer() const;

	// Per-state tick methods — each handles its own transitions.
	void TickIdle(float DeltaTime);
	void TickPatrol(float DeltaTime);
	void TickChase(float DeltaTime);
	void TickAttack(float DeltaTime);

	// Changes state and applies associated movement speed / side-effects.
	void SetEnemyState(EEnemyState NewState);

	// Adds movement input in DirX direction and rotates character to face it.
	void MoveInDirection(float DirX, float Speed);

	// Enables hitbox, plays montage, starts swing and cooldown timers.
	void BeginMeleeAttack();

	// Disables hitbox and clears swing timer. Called by timer or manually.
	UFUNCTION()
	void EndMeleeAttack();

	// Resets bCanAttack and bIsAttacking after AttackCooldown.
	UFUNCTION()
	void ResetAttackCooldown();

	// Clears bPostAttackMoveLocked after AttackCooldown + PostAttackStandStillDuration.
	UFUNCTION()
	void OnPostAttackMoveUnlocked();

	// Clears bRotationLocked after AttackRotationLockDuration.
	UFUNCTION()
	void OnRotationLockExpired();

	// Overlap handler for MeleeHitbox.
	UFUNCTION()
	void OnMeleeHitboxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// Bound to HealthComp->OnDeath.
	UFUNCTION()
	void OnEnemyDeath();

	// Rolls loot table and spawns AWorldItem actors at death location.
	void SpawnLoot();

	// Called by DeathDestroyTimer — removes the actor from the world.
	UFUNCTION()
	void DestroyEnemy();

	// Caches widget instance and makes the health bar visible.
	void ShowHealthBar();

	// Pushes current health percentage to the widget.
	void UpdateHealthBar();

	UPROPERTY()
	UEnemyHealthBarWidget* HealthBarWidgetInstance;
};
