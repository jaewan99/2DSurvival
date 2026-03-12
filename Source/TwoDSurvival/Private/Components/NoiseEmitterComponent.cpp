// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/NoiseEmitterComponent.h"
#include "Enemy/EnemyBase.h"
#include "EngineUtils.h"

UNoiseEmitterComponent::UNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UNoiseEmitterComponent::GetRadiusForType(ENoiseType Type) const
{
	switch (Type)
	{
	case ENoiseType::Footstep:   return FootstepRadius;
	case ENoiseType::Combat:     return CombatRadius;
	case ENoiseType::Door:       return DoorRadius;
	case ENoiseType::ItemPickup: return ItemPickupRadius;
	default:                     return FootstepRadius;
	}
}

void UNoiseEmitterComponent::EmitNoise(ENoiseType Type, float RadiusOverride)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const float Radius = (RadiusOverride > 0.f) ? RadiusOverride : GetRadiusForType(Type);
	BroadcastNoiseAt(Owner->GetWorld(), Owner->GetActorLocation(), Radius);
}

void UNoiseEmitterComponent::BroadcastNoiseAt(UWorld* World, FVector Origin, float Radius)
{
	if (!World) return;

	for (TActorIterator<AEnemyBase> It(World); It; ++It)
	{
		AEnemyBase* Enemy = *It;
		if (!Enemy) continue;

		if (FVector::Dist(Origin, Enemy->GetActorLocation()) <= Radius)
		{
			Enemy->HearNoise(Origin);
		}
	}
}

void UNoiseEmitterComponent::TickFootstep(float DeltaTime, bool bIsMoving, float SpeedFraction)
{
	if (!bIsMoving)
	{
		FootstepAccum = 0.f;
		return;
	}

	FootstepAccum += DeltaTime;

	// Sprinting (SpeedFraction > 1) emits noise more frequently and at a larger radius.
	const float Interval = FootstepInterval / FMath::Max(SpeedFraction, 0.5f);
	if (FootstepAccum >= Interval)
	{
		FootstepAccum = 0.f;
		const float Radius = FootstepRadius * FMath::Clamp(SpeedFraction, 0.5f, 2.f);
		EmitNoise(ENoiseType::Footstep, Radius);
	}
}
