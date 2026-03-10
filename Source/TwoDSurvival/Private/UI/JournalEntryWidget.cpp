// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/JournalEntryWidget.h"
#include "Components/TextBlock.h"

void UJournalEntryWidget::SetEntryData(const FJournalEntry& Entry)
{
	CurrentEntry = Entry;

	if (DateText)
	{
		DateText->SetText(FText::Format(
			NSLOCTEXT("Journal", "EntryDate", "Day {0}, Month {1}, Year {2}"),
			FText::AsNumber(Entry.Day),
			FText::AsNumber(Entry.Month),
			FText::AsNumber(Entry.Year)));
	}

	if (NoteText)
	{
		NoteText->SetText(Entry.NoteText);
	}

	OnEntryRefreshed(Entry);
}
