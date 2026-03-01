// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHealthBarWidget::SetHealthPercent(float Percent)
{
	if (!HealthBar) return;

	HealthBar->SetPercent(Percent);

	FLinearColor Color;
	if      (Percent > 0.6f) Color = FLinearColor(0.1f, 0.8f, 0.1f, 1.f); // Green
	else if (Percent > 0.3f) Color = FLinearColor(0.9f, 0.8f, 0.1f, 1.f); // Yellow
	else                     Color = FLinearColor(0.9f, 0.2f, 0.1f, 1.f); // Red

	HealthBar->SetFillColorAndOpacity(Color);
}
