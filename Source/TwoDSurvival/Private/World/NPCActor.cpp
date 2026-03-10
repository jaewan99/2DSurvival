// Fill out your copyright notice in the Description page of Project Settings.

#include "World/NPCActor.h"
#include "Character/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "World/NPCDefinition.h"
#include "Components/JournalComponent.h"

ANPCActor::ANPCActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetGenerateOverlapEvents(true);
}

void ANPCActor::BeginPlay()
{
	Super::BeginPlay();
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ANPCActor::OnBoxEndOverlap);
}

// --- IInteractable ---

EInteractionType ANPCActor::GetInteractionType_Implementation()
{
	return EInteractionType::Instant;
}

float ANPCActor::GetInteractionDuration_Implementation()
{
	return 0.f;
}

FText ANPCActor::GetInteractionPrompt_Implementation()
{
	if (NPCDef)
	{
		return FText::Format(NSLOCTEXT("NPC", "TalkPrompt", "Talk to {0}"), NPCDef->NPCName);
	}
	return FText::FromString(TEXT("Talk"));
}

void ANPCActor::OnInteract_Implementation(ABaseCharacter* Interactor)
{
	if (!Interactor) return;

	InteractingPlayer = Interactor;
	Interactor->OpenDialogue(this);

	// Auto-add a first-meeting note the very first time the player talks to this NPC.
	if (NPCDef && Interactor->JournalComponent
		&& !Interactor->JournalComponent->HasNoteForNPC(NPCDef->NPCID))
	{
		Interactor->JournalComponent->AddNPCMetNote(NPCDef);
	}
}

// --- Out-of-range close ---

void ANPCActor::OnBoxEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor);
	if (Player && Player == InteractingPlayer.Get())
	{
		Player->CloseDialogue();
		InteractingPlayer = nullptr;
	}
}

// --- Trade completion ---

void ANPCActor::NotifyTradeCompleted()
{
	bTradeCompleted = true;
	OnTradeCompleted.Broadcast(this);

	// Add a trade-complete note to the player's journal.
	if (ABaseCharacter* Player = InteractingPlayer.Get())
	{
		if (NPCDef && Player->JournalComponent)
		{
			Player->JournalComponent->AddNPCTradeNote(NPCDef);
		}
	}
}
