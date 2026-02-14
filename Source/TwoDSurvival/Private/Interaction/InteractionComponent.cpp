// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Character/BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	DetectionSphere = nullptr;
	FocusedInteractable = nullptr;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Create the detection sphere dynamically and attach it to the owner's root.
	DetectionSphere = NewObject<USphereComponent>(Owner, USphereComponent::StaticClass(), TEXT("InteractionDetectionSphere"));
	DetectionSphere->SetSphereRadius(DetectionRadius);
	DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->SetHiddenInGame(true);
	DetectionSphere->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	DetectionSphere->RegisterComponent();

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractionComponent::OnDetectionBeginOverlap);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractionComponent::OnDetectionEndOverlap);
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInteracting || !IsValid(FocusedInteractable)) return;

	const float Duration = IInteractable::Execute_GetInteractionDuration(FocusedInteractable);
	HoldElapsed += DeltaTime;
	InteractionProgress = FMath::Clamp(HoldElapsed / FMath::Max(Duration, SMALL_NUMBER), 0.f, 1.f);

	if (HoldElapsed >= Duration)
	{
		CompleteInteraction();
	}
}

// ---------------------------------------------------------
// Public API
// ---------------------------------------------------------

void UInteractionComponent::StartInteract()
{
	if (bIsInteracting) return;
	if (!IsValid(FocusedInteractable)) return;

	const EInteractionType Type = IInteractable::Execute_GetInteractionType(FocusedInteractable);

	if (Type == EInteractionType::Instant)
	{
		IInteractable::Execute_OnInteract(FocusedInteractable, Cast<ABaseCharacter>(GetOwner()));
	}
	else // Hold
	{
		bIsInteracting = true;
		HoldElapsed = 0.f;
		InteractionProgress = 0.f;

		// Lock movement for the duration of the hold.
		if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
		{
			Char->GetCharacterMovement()->DisableMovement();
		}
	}
}

void UInteractionComponent::StopInteract()
{
	if (bIsInteracting)
	{
		CancelInteraction();
	}
}

bool UInteractionComponent::HasFocusedInteractable() const
{
	return IsValid(FocusedInteractable);
}

// ---------------------------------------------------------
// Overlap callbacks
// ---------------------------------------------------------

void UInteractionComponent::OnDetectionBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (OtherActor && OtherActor->Implements<UInteractable>())
	{
		NearbyInteractables.AddUnique(OtherActor);
		UpdateFocusedInteractable();
	}
}

void UInteractionComponent::OnDetectionEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (!OtherActor) return;

	NearbyInteractables.Remove(OtherActor);

	// If the actor that left was the focused one, cancel any active hold.
	if (FocusedInteractable == OtherActor)
	{
		if (bIsInteracting) CancelInteraction();
		FocusedInteractable = nullptr;
		InteractionPrompt = FText::GetEmpty();
	}

	UpdateFocusedInteractable();
}

// ---------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------

void UInteractionComponent::UpdateFocusedInteractable()
{
	// Purge any stale entries (e.g. destroyed actors).
	NearbyInteractables.RemoveAll([](const AActor* A) { return !IsValid(A); });

	const AActor* Owner = GetOwner();
	if (!Owner || NearbyInteractables.IsEmpty())
	{
		FocusedInteractable = nullptr;
		InteractionPrompt = FText::GetEmpty();
		return;
	}

	const FVector OwnerLoc = Owner->GetActorLocation();
	AActor* Best = nullptr;
	float BestDistSq = MAX_FLT;

	for (AActor* Candidate : NearbyInteractables)
	{
		if (!IsValid(Candidate)) continue;
		const float DistSq = FVector::DistSquared(OwnerLoc, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	FocusedInteractable = Best;
	InteractionPrompt = IsValid(FocusedInteractable)
		? IInteractable::Execute_GetInteractionPrompt(FocusedInteractable)
		: FText::GetEmpty();
}

void UInteractionComponent::CompleteInteraction()
{
	if (IsValid(FocusedInteractable))
	{
		IInteractable::Execute_OnInteract(FocusedInteractable, Cast<ABaseCharacter>(GetOwner()));
	}

	bIsInteracting = false;
	InteractionProgress = 0.f;
	HoldElapsed = 0.f;

	// Restore movement.
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void UInteractionComponent::CancelInteraction()
{
	bIsInteracting = false;
	InteractionProgress = 0.f;
	HoldElapsed = 0.f;

	// Restore movement.
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		Char->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}
