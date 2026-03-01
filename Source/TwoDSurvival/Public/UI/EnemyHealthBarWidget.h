// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

/**
 * World-space health bar widget shown above an enemy.
 * Attached via UWidgetComponent on AEnemyBase.
 * Blueprint child WBP_EnemyHealthBar needs a ProgressBar named "HealthBar".
 */
UCLASS()
class TWODSURVIVAL_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Required widget — name it "HealthBar" in WBP_EnemyHealthBar.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	// Updates bar fill and color. Called by AEnemyBase on every damage hit.
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetHealthPercent(float Percent);
};
