// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/BodyPartRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UBodyPartRowWidget::SetData(FName PartName, float Percent)
{
	if (PartLabel)
	{
		PartLabel->SetText(FText::FromName(PartName));
	}

	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));

		// Color based on health threshold
		FLinearColor BarColor;
		if (Percent <= 0.f)
		{
			BarColor = FLinearColor(0.4f, 0.4f, 0.4f); // Grey — broken
		}
		else if (Percent <= 0.3f)
		{
			BarColor = FLinearColor(0.8f, 0.1f, 0.1f); // Red — critical
		}
		else if (Percent <= 0.6f)
		{
			BarColor = FLinearColor(0.9f, 0.8f, 0.1f); // Yellow — damaged
		}
		else
		{
			BarColor = FLinearColor(0.1f, 0.8f, 0.2f); // Green — healthy
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	if (BrokenText)
	{
		BrokenText->SetVisibility(Percent <= 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
