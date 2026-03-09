// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HotbarWidget.h"
#include "UI/HotbarSlotWidget.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

void UHotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		CachedHotbarComp = Pawn->FindComponentByClass<UHotbarComponent>();
	}

	if (CachedHotbarComp)
	{
		CachedHotbarComp->OnHotbarChanged.AddDynamic(this, &UHotbarWidget::OnHotbarChanged);
	}

	RebuildSlots();
}

void UHotbarWidget::RebuildSlots()
{
	if (!SlotContainer || !WidgetTree) return;

	if (!SlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HotbarWidget] SlotWidgetClass is not set — assign WBP_HotbarSlot in WBP_HotbarWidget class defaults."));
		return;
	}

	const int32 SlotCount = CachedHotbarComp ? CachedHotbarComp->HotbarSlots.Num() : 0;

	// Avoid full rebuild if count hasn't changed.
	if (SlotCount == LastKnownSlotCount)
	{
		RefreshSlots();
		return;
	}
	LastKnownSlotCount = SlotCount;

	SlotContainer->ClearChildren();
	SlotWidgets.Empty();
	SlotWidgets.Reserve(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		UHotbarSlotWidget* SlotWidget = CreateWidget<UHotbarSlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (!SlotWidget) continue;

		UHorizontalBoxSlot* HBSlot = SlotContainer->AddChildToHorizontalBox(SlotWidget);
		if (HBSlot) HBSlot->SetPadding(FMargin(2.f));

		SlotWidgets.Add(SlotWidget);
	}

	RefreshSlots();
}

void UHotbarWidget::RefreshSlots()
{
	if (!CachedHotbarComp) return;

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (!SlotWidgets[i]) continue;

		UItemDefinition* ItemDef = CachedHotbarComp->GetHotbarSlotItem(i);
		const bool bIsActive = (i == CachedHotbarComp->ActiveSlotIndex);
		SlotWidgets[i]->SetSlotData(ItemDef, bIsActive, i);
	}
}

void UHotbarWidget::OnHotbarChanged()
{
	const int32 NewCount = CachedHotbarComp ? CachedHotbarComp->HotbarSlots.Num() : 0;
	if (NewCount != LastKnownSlotCount)
	{
		RebuildSlots();
	}
	else
	{
		RefreshSlots();
	}
}
