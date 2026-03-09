// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NPCDefinition.generated.h"

class UItemDefinition;
class UTexture2D;

/**
 * One trade offer: the player gives RequiredItem × RequiredCount
 * and receives RewardItem × RewardCount.
 * Extend this struct with bUnlocksPath / FName UnlockTag for future unlock logic.
 */
USTRUCT(BlueprintType)
struct FNPCTradeOffer
{
	GENERATED_BODY()

	// Item the player must hand over.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	TObjectPtr<UItemDefinition> RequiredItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	int32 RequiredCount = 1;

	// Item the NPC gives back.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	TObjectPtr<UItemDefinition> RewardItem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	int32 RewardCount = 1;
};

/**
 * Data asset defining an NPC — portrait, dialogue lines, and an optional trade.
 * Create one per NPC type via right-click → Miscellaneous → Data Asset → UNPCDefinition.
 */
UCLASS(BlueprintType)
class TWODSURVIVAL_API UNPCDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Unique ID used to persist trade completion across saves.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	FName NPCID;

	// Display name shown in the dialogue widget header.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	FText NPCName;

	// Portrait texture displayed in the dialogue widget.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UTexture2D> Portrait;

	// Lines cycled through when the player presses Next.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
	TArray<FText> DialogueLines;

	// Whether this NPC offers a one-time item trade.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
	bool bHasTradeOffer = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade",
		meta = (EditCondition = "bHasTradeOffer"))
	FNPCTradeOffer TradeOffer;

	// Dialogue shown after the trade is completed (replaces normal lines).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade",
		meta = (EditCondition = "bHasTradeOffer"))
	FText PostTradeDialogue;
};
