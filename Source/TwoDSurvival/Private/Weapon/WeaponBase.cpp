// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(WeaponMesh);

	// Hitbox is disabled by default — enabled in attack animations
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
