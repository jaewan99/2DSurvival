// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/JournalComponent.h"
#include "World/NPCDefinition.h"
#include "World/TimeManager.h"
#include "Kismet/GameplayStatics.h"

UJournalComponent::UJournalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJournalComponent::AddEntry(FText NoteText, FName SourceNPCID)
{
	FJournalEntry Entry;
	Entry.NoteText    = NoteText;
	Entry.SourceNPCID = SourceNPCID;
	Entry.Day   = 1;
	Entry.Month = 1;
	Entry.Year  = 1;

	// Stamp the current in-game calendar date.
	if (ATimeManager* TM = Cast<ATimeManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ATimeManager::StaticClass())))
	{
		Entry.Day   = TM->CurrentDay;
		Entry.Month = TM->CurrentMonth;
		Entry.Year  = TM->CurrentYear;
	}

	Entries.Add(Entry);
	OnJournalUpdated.Broadcast();
}

bool UJournalComponent::HasNoteForNPC(FName NPCID) const
{
	if (NPCID.IsNone()) return false;
	for (const FJournalEntry& E : Entries)
	{
		if (E.SourceNPCID == NPCID) return true;
	}
	return false;
}

void UJournalComponent::AddNPCMetNote(UNPCDefinition* Def)
{
	if (!Def || Def->NPCID.IsNone()) return;

	FText Note;
	if (Def->bHasTradeOffer && Def->TradeOffer.RequiredItem && Def->TradeOffer.RewardItem)
	{
		Note = FText::Format(
			NSLOCTEXT("Journal", "NPCMet_Trade",
				"Met {0}. They want {1}x {2} in exchange for {3}x {4}."),
			Def->NPCName,
			FText::AsNumber(Def->TradeOffer.RequiredCount),
			Def->TradeOffer.RequiredItem->DisplayName,
			FText::AsNumber(Def->TradeOffer.RewardCount),
			Def->TradeOffer.RewardItem->DisplayName);
	}
	else
	{
		Note = FText::Format(
			NSLOCTEXT("Journal", "NPCMet_Simple", "Met {0}."),
			Def->NPCName);
	}

	AddEntry(Note, Def->NPCID);
}

void UJournalComponent::AddNPCTradeNote(UNPCDefinition* Def)
{
	if (!Def || Def->NPCID.IsNone()) return;
	if (!Def->TradeOffer.RequiredItem || !Def->TradeOffer.RewardItem) return;

	const FText Note = FText::Format(
		NSLOCTEXT("Journal", "NPCTrade",
			"{0}: Trade complete. Gave {1}x {2}, received {3}x {4}."),
		Def->NPCName,
		FText::AsNumber(Def->TradeOffer.RequiredCount),
		Def->TradeOffer.RequiredItem->DisplayName,
		FText::AsNumber(Def->TradeOffer.RewardCount),
		Def->TradeOffer.RewardItem->DisplayName);

	AddEntry(Note, Def->NPCID);
}
