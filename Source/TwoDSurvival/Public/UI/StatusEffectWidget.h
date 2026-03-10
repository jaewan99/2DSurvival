// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/StatusEffectTypes.h"
#include "StatusEffectWidget.generated.h"

class UVerticalBox;
class UStatusEffectComponent;

/**
 * Always-visible strip that shows active status effect names.
 * Blueprint child (WBP_StatusEffects):
 *   - Add VerticalBox named "EffectContainer"
 * Override OnEffectsRefreshed for custom icons or color-coded styling.
 */
UCLASS()
class TWODSURVIVAL_API UStatusEffectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> EffectContainer;

	/** Called after C++ rebuilds the list — override in Blueprint for custom styling. */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffects")
	void OnEffectsRefreshed(const TArray<FActiveStatusEffect>& Effects);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<UStatusEffectComponent> StatusEffectComp;

	void RefreshEffects();

	UFUNCTION()
	void OnStatusEffectsChanged();
};
