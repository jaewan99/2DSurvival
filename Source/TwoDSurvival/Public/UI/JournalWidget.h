// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JournalWidget.generated.h"

class UScrollBox;
class UJournalEntryWidget;
class UJournalComponent;

/**
 * Journal overlay — shows all note entries, newest first.
 * Blueprint child (WBP_JournalWidget):
 *   - Add ScrollBox named "EntryList"
 *   - Set EntryWidgetClass = WBP_JournalEntry in class defaults
 * Toggle with J key.
 */
UCLASS()
class TWODSURVIVAL_API UJournalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Must be named "EntryList" in the Blueprint child. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> EntryList;

	/** Assign WBP_JournalEntry here in class defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "Journal")
	TSubclassOf<UJournalEntryWidget> EntryWidgetClass;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<UJournalComponent> JournalComp;

	void RebuildEntries();

	UFUNCTION()
	void OnJournalUpdated();
};
