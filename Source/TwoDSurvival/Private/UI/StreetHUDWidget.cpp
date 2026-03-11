// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StreetHUDWidget.h"
#include "World/StreetManager.h"
#include "World/StreetDefinition.h"
#include "World/WorldEventManager.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UStreetHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ── Street exit arrows ───────────────────────────────────────────────────
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UStreetManager* SM = GI->GetSubsystem<UStreetManager>())
		{
			SM->OnStreetChanged.AddUObject(this, &UStreetHUDWidget::RefreshArrows);
		}
	}

	// Defer the initial arrow refresh one tick so AStreetBootstrapper::BeginPlay has run.
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UStreetHUDWidget::RefreshArrows);

	// ── World event banner ───────────────────────────────────────────────────
	// Bind to the first AWorldEventManager found in the world.
	TArray<AActor*> Managers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldEventManager::StaticClass(), Managers);
	if (Managers.Num() > 0)
	{
		if (AWorldEventManager* WEM = Cast<AWorldEventManager>(Managers[0]))
		{
			WEM->OnWorldEventStarted.AddDynamic(this, &UStreetHUDWidget::OnWorldEventStarted);
		}
	}

	// Ensure the banner starts hidden.
	if (EventBanner)
	{
		EventBanner->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ── Street arrows ─────────────────────────────────────────────────────────────

void UStreetHUDWidget::RefreshArrows()
{
	UStreetManager* SM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStreetManager>() : nullptr;

	const bool bHasLeft  = SM && SM->CurrentStreet && SM->CurrentStreet->GetExit(FName("Left"))  != nullptr && SM->CurrentStreet->GetExit(FName("Left"))->Destination  != nullptr;
	const bool bHasRight = SM && SM->CurrentStreet && SM->CurrentStreet->GetExit(FName("Right")) != nullptr && SM->CurrentStreet->GetExit(FName("Right"))->Destination != nullptr;
	const bool bHasUp    = SM && SM->CurrentStreet && SM->CurrentStreet->GetExit(FName("Up"))    != nullptr && SM->CurrentStreet->GetExit(FName("Up"))->Destination    != nullptr;

	if (ArrowLeft)  ArrowLeft->SetVisibility(bHasLeft  ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (ArrowRight) ArrowRight->SetVisibility(bHasRight ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (ArrowUp)    ArrowUp->SetVisibility(bHasUp    ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// ── World event notification ──────────────────────────────────────────────────

void UStreetHUDWidget::OnWorldEventStarted(EWorldEventType EventType, FText BannerText)
{
	ShowNotification(BannerText, 5.f);
}

void UStreetHUDWidget::ShowNotification(const FText& BannerText, float Duration)
{
	if (!EventBanner) return;

	EventBanner->SetText(BannerText);
	EventBanner->SetVisibility(ESlateVisibility::HitTestInvisible);

	// (Re)start the hide timer — calling this again while a banner is showing
	// simply extends / replaces the previous message cleanly.
	GetWorld()->GetTimerManager().SetTimer(
		NotificationTimer,
		this,
		&UStreetHUDWidget::HideNotification,
		FMath::Max(0.1f, Duration),
		/*bLoop=*/ false);
}

void UStreetHUDWidget::HideNotification()
{
	if (EventBanner)
	{
		EventBanner->SetVisibility(ESlateVisibility::Collapsed);
	}
}
