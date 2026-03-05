// Fill out your copyright notice in the Description page of Project Settings.

#include "World/DraggableProp.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/BaseCharacter.h"

ADraggableProp::ADraggableProp()
{
	PrimaryActorTick.bCanEverTick = false;

	// Mesh is the root; disable its own collision — CollisionBox handles all blocking.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Solid blocking volume — keeps the prop colliding with walls, floors, other actors.
	// Resize extents in the Blueprint child to match the mesh.
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(Mesh);
	CollisionBox->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

	// Slightly larger overlap volume — detected by UInteractionComponent's detection sphere.
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Mesh);
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetBoxExtent(FVector(80.f, 80.f, 80.f));
}

EInteractionType ADraggableProp::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float ADraggableProp::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText ADraggableProp::GetInteractionPrompt_Implementation()
{
	// "Push" is shown when idle. While dragging, InteractionComponent overrides
	// the prompt to "Release" via LockFocus() — this function won't be called then.
	return FText::FromString(TEXT("Push"));
}

void ADraggableProp::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor) return;

	if (Interactor->GrabbedProp == this)
	{
		Interactor->ReleaseDrag();
	}
	else
	{
		Interactor->StartDrag(this);
	}
}
