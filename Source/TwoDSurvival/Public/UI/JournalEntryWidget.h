// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/JournalComponent.h"  // FJournalEntry
#include "JournalEntryWidget.generated.h"

class UTextBlock;

/**
 * Widget representing one note entry in the journal scroll list.
 * Blueprint child (WBP_JournalEntry) — optionally add named widgets for auto-population:
 *   - TextBlock "DateText"  — filled with "Day X, Month Y, Year Z"
 *   - TextBlock "NoteText"  — filled with the note body
 * Override OnEntryRefreshed in Blueprint for custom styling (e.g. different colors per NPC).
 */
UCLASS()
class TWODSURVIVAL_API UJournalEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populate the widget from a journal entry. */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void SetEntryData(const FJournalEntry& Entry);

	/** Called after SetEntryData — override in Blueprint for custom styling. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Journal")
	void OnEntryRefreshed(const FJournalEntry& Entry);

	/** The entry data currently displayed, readable from Blueprint overrides. */
	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	FJournalEntry CurrentEntry;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoteText;
};
