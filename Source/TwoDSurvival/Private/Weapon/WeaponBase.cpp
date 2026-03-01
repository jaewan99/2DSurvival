// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Combat/DamageableInterface.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(WeaponMesh);

	// Hitbox is off by default — enabled per-swing in BeginAttack
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetGenerateOverlapEvents(false);

	// Only overlap with Pawns (enemies) — ignore everything else
	HitboxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnHitboxOverlap);
}

void AWeaponBase::BeginAttack(float DamageMultiplier)
{
	// Guard: already active — e.g. OnAttackPressed called this, then AnimNotify_BeginAttack fired too.
	// Let the first call's timer handle EndAttack; the second call is a no-op.
	if (HitboxComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision) return;

	StoredDamageMultiplier = DamageMultiplier;
	HitActorsThisSwing.Reset();

	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitboxComponent->SetGenerateOverlapEvents(true);

	GetWorldTimerManager().SetTimer(SwingTimerHandle, this, &AWeaponBase::EndAttack, SwingWindowDuration, false);
}

void AWeaponBase::EndAttack()
{
	// Clear the timer in case EndAttack was called early (e.g. by AnimNotify_EndAttack)
	GetWorldTimerManager().ClearTimer(SwingTimerHandle);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetGenerateOverlapEvents(false);
	HitActorsThisSwing.Reset();
}

void AWeaponBase::OnHitboxOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Don't hit the character who is swinging this weapon
	if (OtherActor == GetOwner()) return;

	// One hit per actor per swing
	if (HitActorsThisSwing.Contains(OtherActor)) return;

	// Only damage actors that implement IDamageable
	if (!OtherActor->GetClass()->ImplementsInterface(UDamageable::StaticClass())) return;

	HitActorsThisSwing.Add(OtherActor);

	const float FinalDamage = BaseDamage * StoredDamageMultiplier;
	IDamageable::Execute_TakeMeleeDamage(OtherActor, FinalDamage, GetOwner());

	UE_LOG(LogTemp, Log, TEXT("WeaponBase: Hit %s for %.1f damage"), *OtherActor->GetName(), FinalDamage);
}
