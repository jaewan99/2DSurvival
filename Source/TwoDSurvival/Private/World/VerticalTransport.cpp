// Fill out your copyright notice in the Description page of Project Settings.

#include "World/VerticalTransport.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/BaseCharacter.h"

AVerticalTransport::AVerticalTransport()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Mesh — assign the stair/ladder asset in the Blueprint Details panel.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootScene);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // CollisionBox handles blocking

	// Physical collision box, child of Mesh so it moves with any mesh offset.
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(Mesh);
	CollisionBox->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionBox->SetGenerateOverlapEvents(false);

	// Interaction trigger — overlap only, spans both floor levels for E-press detection.
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Mesh);
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetGenerateOverlapEvents(true);

	// Exit markers — drag these to the landing positions in the Blueprint viewport.
	LowerExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LowerExitPoint"));
	LowerExitPoint->SetupAttachment(RootScene);
	LowerExitPoint->SetRelativeLocation(FVector(0.f, 0.f, 0.f)); // foot of stairs by default

	UpperExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("UpperExitPoint"));
	UpperExitPoint->SetupAttachment(RootScene);
	UpperExitPoint->SetRelativeLocation(FVector(0.f, 0.f, 400.f)); // top of stairs by default

	// Default sizes for FloorHeight=400. ResizeBoxes() updates both at BeginPlay.
	ResizeBoxes();
}

void AVerticalTransport::BeginPlay()
{
	Super::BeginPlay();

	// Apply FloorHeight that may have been set by ABuildingGenerator before BeginPlay.
	ResizeBoxes();

	InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AVerticalTransport::OnOverlapBegin);
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AVerticalTransport::OnOverlapEnd);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

void AVerticalTransport::SetFloorHeight(float NewFloorHeight)
{
	FloorHeight = NewFloorHeight;
	ResizeBoxes();
}

FText AVerticalTransport::GetInteractionPrompt_Implementation()
{
	const bool bGoUp = IsPlayerOnLowerFloor(NearbyPlayer.Get());

	if (TransportType == ETransportType::Ladder)
	{
		return FText::FromString(bGoUp ? TEXT("Climb Up") : TEXT("Climb Down"));
	}
	return FText::FromString(bGoUp ? TEXT("Go Upstairs") : TEXT("Go Downstairs"));
}

void AVerticalTransport::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor) return;

	// Use the designer-placed exit markers for exact landing positions.
	const FVector Destination = IsPlayerOnLowerFloor(Interactor)
		? UpperExitPoint->GetComponentLocation()
		: LowerExitPoint->GetComponentLocation();

	Interactor->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private
// ─────────────────────────────────────────────────────────────────────────────

void AVerticalTransport::ResizeBoxes()
{
	const float HalfH = FloorHeight * 0.5f;

	// InteractionBox: overlap trigger spanning both floor levels.
	// Centered at (RoomWidth/2, 0, FloorHeight/2) so it covers the full cell.
	InteractionBox->SetBoxExtent(FVector(400.f, 60.f, HalfH));
	InteractionBox->SetRelativeLocation(FVector(400.f, 0.f, HalfH));

	// CollisionBox: physical blocker matching the same cell dimensions.
	// Relative to Mesh (which is at RootScene origin), so same offsets apply.
	CollisionBox->SetBoxExtent(FVector(400.f, 30.f, HalfH));
	CollisionBox->SetRelativeLocation(FVector(400.f, 0.f, HalfH));
}

bool AVerticalTransport::IsPlayerOnLowerFloor(ABaseCharacter* Player) const
{
	if (!Player) return true;
	// Midpoint between the two floors
	const float Midpoint = GetActorLocation().Z + FloorHeight * 0.5f;
	return Player->GetActorLocation().Z < Midpoint;
}

void AVerticalTransport::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
	{
		NearbyPlayer = Player;
	}
}

void AVerticalTransport::OnOverlapEnd(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (Cast<ABaseCharacter>(OtherActor))
	{
		NearbyPlayer = nullptr;
	}
}
