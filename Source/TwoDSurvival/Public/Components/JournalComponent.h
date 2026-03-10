// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JournalComponent.generated.h"

class UNPCDefinition;

/**
 * One note entry in the player's journal.
 * Auto-generated from NPC interactions; also supports manual entries.
 */
USTRUCT(BlueprintType)
struct FJournalEntry
{
	GENERATED_BODY()

	/** The note text shown in the journal. */
	UPROPERTY(BlueprintReadOnly)
	FText NoteText;

	/** Which NPC triggered this note. NAME_None for non-NPC entries. */
	UPROPERTY(BlueprintReadOnly)
	FName SourceNPCID;

	UPROPERTY(BlueprintReadOnly)
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 Month = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 Year = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalUpdated);

/**
 * Tracks player notes about NPC exchanges and world events.
 * Auto-populates on first NPC meeting and on trade completion.
 * Toggle the journal UI with J.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UJournalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJournalComponent();

	/** All recorded notes, oldest first. */
	UPROPERTY(BlueprintReadOnly, Category = "Journal")
	TArray<FJournalEntry> Entries;

	/** Broadcast whenever a new entry is added. */
	UPROPERTY(BlueprintAssignable, Category = "Journal")
	FOnJournalUpdated OnJournalUpdated;

	/**
	 * Add a note. Timestamps from the current in-game date (via ATimeManager).
	 * SourceNPCID identifies which NPC the note is about (NAME_None if none).
	 */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void AddEntry(FText NoteText, FName SourceNPCID = NAME_None);

	/** True if any entry already references this NPC — prevents duplicate first-meet notes. */
	bool HasNoteForNPC(FName NPCID) const;

	/** Auto-generates a "first meeting" note from the NPC definition. */
	void AddNPCMetNote(UNPCDefinition* Def);

	/** Auto-generates a "trade complete" note from the NPC definition. */
	void AddNPCTradeNote(UNPCDefinition* Def);
};
