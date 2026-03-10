// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/JournalWidget.h"
#include "UI/JournalEntryWidget.h"
#include "Components/JournalComponent.h"
#include "Components/ScrollBox.h"
#include "Character/BaseCharacter.h"

void UJournalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (Player)
	{
		JournalComp = Player->JournalComponent;
		if (JournalComp)
		{
			JournalComp->OnJournalUpdated.AddDynamic(this, &UJournalWidget::OnJournalUpdated);
		}
	}

	RebuildEntries();
}

void UJournalWidget::RebuildEntries()
{
	if (!EntryList || !EntryWidgetClass || !JournalComp) return;

	EntryList->ClearChildren();

	// Show newest first.
	const TArray<FJournalEntry>& AllEntries = JournalComp->Entries;
	for (int32 i = AllEntries.Num() - 1; i >= 0; --i)
	{
		UJournalEntryWidget* EntryWidget = CreateWidget<UJournalEntryWidget>(
			GetOwningPlayer(), EntryWidgetClass);
		if (EntryWidget)
		{
			EntryWidget->SetEntryData(AllEntries[i]);
			EntryList->AddChild(EntryWidget);
		}
	}
}

void UJournalWidget::OnJournalUpdated()
{
	RebuildEntries();
}
