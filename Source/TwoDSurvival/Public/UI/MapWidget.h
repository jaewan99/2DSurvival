// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapWidget.generated.h"

class UCanvasPanel;
class UButton;
class UStreetDefinition;
class UCityDefinition;

/**
 * World map widget — shows visited streets as nodes connected by lines.
 * Streets are grouped into city regions with colored backgrounds.
 * Highway segments between cities are shown with amber lines.
 * Current street is highlighted gold. Buildings shown as bullet labels per node.
 *
 * Blueprint child (WBP_MapWidget) needs:
 *   - CanvasPanel  "MapCanvas"    — fills the widget area (800×500 recommended)
 *   - Button       "CloseButton"  — close the map
 *
 * Assign MapWidgetClass on BP_BaseCharacter. Toggle with M key.
 */
UCLASS()
class TWODSURVIVAL_API UMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MapCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

private:
	// Plain C++ struct — not reflected, used only in NativePaint.
	struct FCityRegion
	{
		FLinearColor BorderColor;  // city's MapColor, full opacity
		FVector2D    BoxMin;       // canvas top-left
		FVector2D    BoxMax;       // canvas bottom-right
	};

	// Node center positions (canvas local space) — used by NativePaint for line endpoints.
	TMap<FName, FVector2D> NodeCenters;

	// Street-to-street connections (grey).
	TArray<TPair<FVector2D, FVector2D>> ConnectionLines;

	// Connections where at least one side is a highway (amber).
	TArray<TPair<FVector2D, FVector2D>> HighwayLines;

	// City region outlines drawn in NativePaint.
	TArray<FCityRegion> CityRegions;

	// BFS layout + widget creation — called once in NativeConstruct.
	void BuildMap();

	// Creates one street node widget on MapCanvas.
	void CreateNode(UStreetDefinition* Street, FVector2D CanvasPos,
		bool bVisited, bool bIsCurrent);

	// Creates a city background border + name label on MapCanvas (added before nodes).
	void CreateCityBackground(UCityDefinition* City,
		FVector2D BoxMin, FVector2D BoxMax);

	UFUNCTION()
	void OnCloseClicked();

	static constexpr float NodeWidth     = 170.f;
	static constexpr float NodeHeight    = 90.f;
	static constexpr float CellWidth     = 210.f;   // horizontal spacing between node centers
	static constexpr float CanvasOriginX = 450.f;   // canvas X for grid column 0
	static constexpr float CanvasOriginY = 170.f;   // canvas Y for all street nodes
	static constexpr float CityPadX      = 18.f;    // horizontal padding around city group
	static constexpr float CityPadTop    = 30.f;    // space above nodes for city name label
	static constexpr float CityPadBot    = 12.f;    // space below nodes
};
