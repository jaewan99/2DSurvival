// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StreetHUDWidget.generated.h"

class UImage;

/**
 * Always-visible HUD showing directional arrows for available street exits.
 * Reads exit availability from UStreetManager::CurrentStreet — no actor scanning needed.
 * Refreshes automatically via UStreetManager::OnStreetChanged delegate.
 *
 * Blueprint child (WBP_StreetHUD) needs:
 *   - UImage named "ArrowLeft"   — anchor to left edge
 *   - UImage named "ArrowRight"  — anchor to right edge
 *   - UImage named "ArrowUp"     — anchor to top center
 * Assign arrow textures in the Image Details panel.
 */
UCLASS()
class TWODSURVIVAL_API UStreetHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowUp;

	// Reads CurrentStreet exits from UStreetManager and shows/collapses arrows.
	UFUNCTION()
	void RefreshArrows();
};
