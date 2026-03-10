// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StatusEffectWidget.h"
#include "Components/StatusEffectComponent.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Character/BaseCharacter.h"

// Display name for each status effect shown in the widget.
static FText GetEffectLabel(EStatusEffect Effect)
{
	switch (Effect)
	{
	case EStatusEffect::Bleeding:    return FText::FromString(TEXT("Bleeding"));
	case EStatusEffect::Infected:    return FText::FromString(TEXT("Infected"));
	case EStatusEffect::Poisoned:    return FText::FromString(TEXT("Poisoned"));
	case EStatusEffect::BrokenBone:  return FText::FromString(TEXT("Broken Bone"));
	case EStatusEffect::Frostbite:   return FText::FromString(TEXT("Frostbite"));
	case EStatusEffect::Hypothermia: return FText::FromString(TEXT("Hypothermia"));
	case EStatusEffect::Wet:         return FText::FromString(TEXT("Wet"));
	case EStatusEffect::Concussion:  return FText::FromString(TEXT("Concussion"));
	default:                         return FText::FromString(TEXT("Unknown"));
	}
}

void UStatusEffectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (Player)
	{
		StatusEffectComp = Player->StatusEffectComponent;
		if (StatusEffectComp)
		{
			StatusEffectComp->OnStatusEffectsChanged.AddDynamic(
				this, &UStatusEffectWidget::OnStatusEffectsChanged);
		}
	}

	RefreshEffects();
}

void UStatusEffectWidget::RefreshEffects()
{
	if (!EffectContainer) return;

	EffectContainer->ClearChildren();

	if (!StatusEffectComp) return;

	for (const FActiveStatusEffect& Fx : StatusEffectComp->ActiveEffects)
	{
		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(GetEffectLabel(Fx.Type));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.3f, 0.3f)));
		EffectContainer->AddChildToVerticalBox(Label);
	}

	// Hide the container entirely when no effects are active.
	EffectContainer->SetVisibility(
		StatusEffectComp->ActiveEffects.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);

	OnEffectsRefreshed(StatusEffectComp->ActiveEffects);
}

void UStatusEffectWidget::OnStatusEffectsChanged()
{
	RefreshEffects();
}
