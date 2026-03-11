// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SkillComponent.h"
#include "SkillHUDWidget.generated.h"

class USkillRowWidget;
class USkillComponent;
class UBorder;

/**
 * Draggable HUD panel showing Combat, Crafting, and Scavenging skill levels + XP bars.
 * Mirrors the Health HUD pattern — toggled via K key, draggable via TitleBar.
 *
 * Blueprint child (WBP_SkillHUD) needs:
 *   Row_Combat     (WBP_SkillRow)
 *   Row_Crafting   (WBP_SkillRow)
 *   Row_Scavenging (WBP_SkillRow)
 *   TitleBar       (Border, optional — enables drag-to-move)
 */
UCLASS()
class TWODSURVIVAL_API USkillHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USkillRowWidget* Row_Combat;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USkillRowWidget* Row_Crafting;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USkillRowWidget* Row_Scavenging;

	/** Optional drag handle — same pattern as the Health HUD TitleBar. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* TitleBar;

	/** Refresh all three rows from the cached SkillComponent. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshAll();

	/** Called by BaseCharacter after AddToViewport to sync the drag position. */
	void InitDragPosition(FVector2D ViewportPos);

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	USkillComponent* CachedSkillComp;

	// Bound to USkillComponent delegates.
	UFUNCTION()
	void OnSkillXPChanged(ESkillType Skill, int32 CurrentXP, int32 XPForNext);

	UFUNCTION()
	void OnSkillLevelUp(ESkillType Skill, int32 NewLevel);

	/** Returns the row widget for a given skill type. Null if unrecognised. */
	USkillRowWidget* GetRowForSkill(ESkillType Skill) const;

	bool      bIsDragging      = false;
	FVector2D LastMousePos;
	FVector2D WidgetViewportPos;
};
