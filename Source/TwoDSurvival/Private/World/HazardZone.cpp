// Fill out your copyright notice in the Description page of Project Settings.

#include "World/HazardZone.h"
#include "Character/BaseCharacter.h"
#include "Character/HealthComponent.h"
#include "Character/HealthTypes.h"
#include "Components/StatusEffectComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AHazardZone::AHazardZone()
{
	PrimaryActorTick.bCanEverTick = false;

	HazardBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardBox"));
	HazardBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = HazardBox;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AmbientAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
	AmbientAudio->SetupAttachment(RootComponent);
	AmbientAudio->bAutoActivate = false;
}

void AHazardZone::BeginPlay()
{
	Super::BeginPlay();

	HazardBox->OnComponentBeginOverlap.AddDynamic(this, &AHazardZone::OnBoxBeginOverlap);
	HazardBox->OnComponentEndOverlap.AddDynamic(this, &AHazardZone::OnBoxEndOverlap);

	if (SFX_Ambient)
	{
		AmbientAudio->SetSound(SFX_Ambient);
	}
}

void AHazardZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	OverlappingPlayer = Player;

	// Apply once immediately so the player feels it right away, then start the interval timer.
	ApplyHazardEffects();

	GetWorldTimerManager().SetTimer(
		EffectTimerHandle,
		this, &AHazardZone::ApplyHazardEffects,
		EffectInterval,
		/*bLoop=*/ true);

	if (SFX_Ambient)
	{
		AmbientAudio->Play();
	}
}

void AHazardZone::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != OverlappingPlayer)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(EffectTimerHandle);
	OverlappingPlayer = nullptr;

	AmbientAudio->Stop();
}

void AHazardZone::ApplyHazardEffects()
{
	if (!OverlappingPlayer)
	{
		return;
	}

	// Apply status effects — indefinite duration so they persist until cured.
	if (UStatusEffectComponent* SEC = OverlappingPlayer->StatusEffectComponent)
	{
		for (EStatusEffect Effect : EffectsToApply)
		{
			if (Effect != EStatusEffect::None)
			{
				SEC->ApplyEffect(Effect, /*Severity=*/ 1.f, /*Duration=*/ -1.f);
			}
		}
	}

	// Apply direct body damage if configured.
	if (DirectDamagePerInterval > 0.f)
	{
		if (UHealthComponent* HC = OverlappingPlayer->HealthComponent)
		{
			HC->ApplyDamage(EBodyPart::Body, DirectDamagePerInterval);
		}
	}
}
