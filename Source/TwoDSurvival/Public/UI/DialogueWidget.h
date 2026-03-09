// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class ANPCActor;
class ABaseCharacter;
class USoundBase;

/**
 * Dialogue UI shown when the player talks to an NPC.
 *
 * Blueprint child (WBP_DialogueWidget) needs these named widgets:
 *   - Image       "PortraitImage"     — NPC portrait
 *   - TextBlock   "NPCNameText"       — NPC display name
 *   - TextBlock   "DialogueText"      — current dialogue line
 *   - Button      "NextButton"        — advance to next line
 *   - Button      "GiveItemButton"    — give item to NPC (collapsed when not applicable)
 *   - TextBlock   "GiveItemLabel"     — describes the trade (e.g. "Give Apple × 2 → Key")
 *   - Button      "CloseButton"       — close the dialogue
 *
 * Assign WBP_DialogueWidget to DialogueWidgetClass on BP_BaseCharacter.
 */
UCLASS()
class TWODSURVIVAL_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Called by BaseCharacter::OpenDialogue_Implementation after creating the widget.
	void SetNPC(ANPCActor* InNPCActor, ABaseCharacter* InOwner);

	/** Played when the player advances to the next dialogue line. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_NextLine;

	/** Played when the trade is successfully completed. */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> SFX_TradeComplete;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NPCNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextButton;

	// Collapsed when: trade not offered, trade already done, or player lacks the item.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GiveItemButton;

	// Label showing what the player gives and what they receive.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GiveItemLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

private:
	UPROPERTY()
	TObjectPtr<ANPCActor> NPCActor;

	UPROPERTY()
	TObjectPtr<ABaseCharacter> OwnerChar;

	int32 CurrentLineIndex = 0;

	// Pushes the current dialogue line to DialogueText and updates button states.
	void RefreshDialogue();

	// Shows or hides the Give Item button based on trade availability + inventory check.
	void RefreshGiveButton();

	UFUNCTION()
	void OnNextClicked();

	UFUNCTION()
	void OnGiveItemClicked();

	UFUNCTION()
	void OnCloseClicked();
};
