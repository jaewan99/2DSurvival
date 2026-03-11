// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/WorldEventManager.h"
#include "StreetHUDWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * Always-visible HUD showing directional arrows for available street exits
 * and a timed notification banner for random world events.
 *
 * Blueprint child (WBP_StreetHUD) needs:
 *   - UImage named "ArrowLeft"    — anchor to left edge
 *   - UImage named "ArrowRight"   — anchor to right edge
 *   - UImage named "ArrowUp"      — anchor to top center
 *   - UTextBlock named "EventBanner" (optional) — top-center banner, collapsed by default.
 *     Set Visibility = Collapsed in the widget defaults; C++ shows/hides it automatically.
 * Assign arrow textures in the Image Details panel.
 */
UCLASS()
class TWODSURVIVAL_API UStreetHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows BannerText in the EventBanner TextBlock for the given duration, then hides it.
	 * Called automatically by AWorldEventManager via delegate binding in NativeConstruct.
	 * Safe to call when EventBanner is absent (no-op in that case).
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowNotification(const FText& BannerText, float Duration);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowUp;

	/** Optional notification banner shown when a world event fires. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventBanner;

	// Reads CurrentStreet exits from UStreetManager and shows/collapses arrows.
	UFUNCTION()
	void RefreshArrows();

	// Called by AWorldEventManager::OnWorldEventStarted delegate.
	UFUNCTION()
	void OnWorldEventStarted(EWorldEventType EventType, FText BannerText);

	// Hides the EventBanner after the display duration elapses.
	UFUNCTION()
	void HideNotification();

	FTimerHandle NotificationTimer;
};
