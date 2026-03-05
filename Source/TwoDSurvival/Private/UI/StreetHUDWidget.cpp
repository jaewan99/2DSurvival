// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StreetHUDWidget.h"
#include "World/StreetManager.h"
#include "World/StreetDefinition.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"

void UStreetHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind to the manager so arrows refresh automatically on every street change
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			SM->OnStreetChanged.AddUObject(this, &UStreetHUDWidget::RefreshArrows);
		}
	}

	// Defer the initial refresh one tick so AStreetBootstrapper::BeginPlay has run
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UStreetHUDWidget::RefreshArrows);
}

void UStreetHUDWidget::RefreshArrows()
{
	UStreetManager* SM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStreetManager>() : nullptr;

	const bool bHasLeft  = SM && SM->CurrentStreet && SM->CurrentStreet->ExitLeft  != nullptr;
	const bool bHasRight = SM && SM->CurrentStreet && SM->CurrentStreet->ExitRight != nullptr;
	const bool bHasUp    = SM && SM->CurrentStreet && SM->CurrentStreet->ExitUp    != nullptr;

	if (ArrowLeft)  ArrowLeft->SetVisibility(bHasLeft  ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (ArrowRight) ArrowRight->SetVisibility(bHasRight ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (ArrowUp)    ArrowUp->SetVisibility(bHasUp    ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
