// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DialogueWidget.h"
#include "World/NPCActor.h"
#include "Kismet/GameplayStatics.h"
#include "World/NPCDefinition.h"
#include "Character/BaseCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"

void UDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NextButton->OnClicked.AddDynamic(this, &UDialogueWidget::OnNextClicked);
	GiveItemButton->OnClicked.AddDynamic(this, &UDialogueWidget::OnGiveItemClicked);
	CloseButton->OnClicked.AddDynamic(this, &UDialogueWidget::OnCloseClicked);

	// Give item button starts collapsed — SetNPC will reveal it if applicable.
	GiveItemButton->SetVisibility(ESlateVisibility::Collapsed);
	GiveItemLabel->SetVisibility(ESlateVisibility::Collapsed);
}

void UDialogueWidget::SetNPC(ANPCActor* InNPCActor, ABaseCharacter* InOwner)
{
	NPCActor = InNPCActor;
	OwnerChar = InOwner;
	CurrentLineIndex = 0;

	if (!NPCActor || !NPCActor->NPCDef) return;

	UNPCDefinition* Def = NPCActor->NPCDef;

	// Portrait
	if (Def->Portrait)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Def->Portrait);
		Brush.ImageSize = FVector2D(256.f, 256.f);
		PortraitImage->SetBrush(Brush);
	}

	// Name
	NPCNameText->SetText(Def->NPCName);

	RefreshDialogue();
	RefreshGiveButton();
}

void UDialogueWidget::RefreshDialogue()
{
	if (!NPCActor || !NPCActor->NPCDef) return;

	UNPCDefinition* Def = NPCActor->NPCDef;

	// If trade is done, show the post-trade line instead.
	if (NPCActor->bTradeCompleted && Def->bHasTradeOffer)
	{
		DialogueText->SetText(Def->PostTradeDialogue);
		return;
	}

	if (Def->DialogueLines.Num() == 0)
	{
		DialogueText->SetText(FText::FromString(TEXT("...")));
		return;
	}

	DialogueText->SetText(Def->DialogueLines[CurrentLineIndex]);
}

void UDialogueWidget::RefreshGiveButton()
{
	if (!NPCActor || !NPCActor->NPCDef || !OwnerChar) return;

	UNPCDefinition* Def = NPCActor->NPCDef;

	// Hide button if: no trade, trade already done.
	if (!Def->bHasTradeOffer || NPCActor->bTradeCompleted)
	{
		GiveItemButton->SetVisibility(ESlateVisibility::Collapsed);
		GiveItemLabel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FNPCTradeOffer& Offer = Def->TradeOffer;
	if (!Offer.RequiredItem || !Offer.RewardItem)
	{
		GiveItemButton->SetVisibility(ESlateVisibility::Collapsed);
		GiveItemLabel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Check if player has the required items.
	const int32 HaveCount = OwnerChar->InventoryComponent->CountItemByID(Offer.RequiredItem->ItemID);
	const bool bCanTrade = HaveCount >= Offer.RequiredCount;

	GiveItemButton->SetVisibility(ESlateVisibility::Visible);
	GiveItemLabel->SetVisibility(ESlateVisibility::Visible);
	GiveItemButton->SetIsEnabled(bCanTrade);

	// Label: "Give Apple × 2  →  Key × 1"
	FText Label = FText::Format(
		NSLOCTEXT("Dialogue", "TradeLabel", "Give {0} \u00d7 {1}  \u2192  {2} \u00d7 {3}"),
		FText::FromName(Offer.RequiredItem->ItemID),
		FText::AsNumber(Offer.RequiredCount),
		FText::FromName(Offer.RewardItem->ItemID),
		FText::AsNumber(Offer.RewardCount)
	);
	GiveItemLabel->SetText(Label);
}

// --- Button handlers ---

void UDialogueWidget::OnNextClicked()
{
	if (!NPCActor || !NPCActor->NPCDef) return;

	UNPCDefinition* Def = NPCActor->NPCDef;

	if (NPCActor->bTradeCompleted || Def->DialogueLines.Num() == 0) return;

	CurrentLineIndex = (CurrentLineIndex + 1) % Def->DialogueLines.Num();
	UGameplayStatics::PlaySound2D(this, SFX_NextLine);
	RefreshDialogue();
}

void UDialogueWidget::OnGiveItemClicked()
{
	if (!NPCActor || !NPCActor->NPCDef || !OwnerChar) return;
	if (NPCActor->bTradeCompleted) return;

	UNPCDefinition* Def = NPCActor->NPCDef;
	const FNPCTradeOffer& Offer = Def->TradeOffer;

	if (!Offer.RequiredItem || !Offer.RewardItem) return;

	// Verify player still has the item (race-condition guard).
	const int32 HaveCount = OwnerChar->InventoryComponent->CountItemByID(Offer.RequiredItem->ItemID);
	if (HaveCount < Offer.RequiredCount) return;

	// Consume required items.
	OwnerChar->InventoryComponent->RemoveItemByID(Offer.RequiredItem->ItemID, Offer.RequiredCount);

	// Give reward items.
	OwnerChar->InventoryComponent->TryAddItem(Offer.RewardItem, Offer.RewardCount);

	// Mark trade done on the NPC — fires OnTradeCompleted delegate for Blueprint unlock hooks.
	NPCActor->NotifyTradeCompleted();
	UGameplayStatics::PlaySound2D(this, SFX_TradeComplete);

	// Switch to post-trade dialogue and hide the give button.
	RefreshDialogue();
	RefreshGiveButton();
}

void UDialogueWidget::OnCloseClicked()
{
	if (OwnerChar)
	{
		OwnerChar->CloseDialogue();
	}
}
