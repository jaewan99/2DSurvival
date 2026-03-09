// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MapWidget.h"
#include "World/StreetManager.h"
#include "World/StreetDefinition.h"
#include "World/CityDefinition.h"
#include "Character/BaseCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"

void UMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CloseButton->OnClicked.AddDynamic(this, &UMapWidget::OnCloseClicked);

	BuildMap();
}

void UMapWidget::BuildMap()
{
	UGameInstance* GI = GetOwningPlayer() ? GetOwningPlayer()->GetGameInstance() : nullptr;
	if (!GI) return;

	UStreetManager* SM = GI->GetSubsystem<UStreetManager>();
	if (!SM) return;

	UStreetDefinition* Start = SM->GetStartingStreet();
	if (!Start) return;

	const TSet<FName>& Visited  = SM->GetVisitedStreets();
	const FName        CurrentID = SM->CurrentStreet ? SM->CurrentStreet->StreetID : NAME_None;

	// ── BFS: assign a grid column (float) to every reachable street ──────────
	TMap<FName, float>            GridX;    // StreetID → column
	TMap<FName, UStreetDefinition*> AllDefs; // StreetID → definition
	TQueue<UStreetDefinition*>    Queue;

	GridX.Add(Start->StreetID, 0.f);
	AllDefs.Add(Start->StreetID, Start);
	Queue.Enqueue(Start);

	while (!Queue.IsEmpty())
	{
		UStreetDefinition* Street;
		Queue.Dequeue(Street);
		const float X = GridX[Street->StreetID];

		for (const FStreetExitLink& Exit : Street->Exits)
		{
			if (!Exit.Destination) continue;
			if (Exit.Layout == EExitLayout::Building)
			{
				// Buildings are labels on their parent node, not separate map nodes.
				AllDefs.FindOrAdd(Exit.Destination->StreetID, Exit.Destination);
				continue;
			}
			if (GridX.Contains(Exit.Destination->StreetID)) continue;

			const float NextX = X + (Exit.Layout == EExitLayout::AdjacentRight ? 1.f : -1.f);
			GridX.Add(Exit.Destination->StreetID, NextX);
			AllDefs.Add(Exit.Destination->StreetID, Exit.Destination);
			Queue.Enqueue(Exit.Destination);
		}
	}

	// ── Pre-compute canvas positions (needed before creating any widgets) ────
	TMap<FName, FVector2D> NodeTopLeft; // StreetID → canvas top-left of the node box

	for (auto& KV : GridX)
	{
		const float CanvasX = CanvasOriginX + KV.Value * CellWidth - NodeWidth * 0.5f;
		NodeTopLeft.Add(KV.Key, FVector2D(CanvasX, CanvasOriginY));
		NodeCenters.Add(KV.Key, FVector2D(CanvasX + NodeWidth * 0.5f, CanvasOriginY + NodeHeight * 0.5f));
	}

	// ── Compute city bounding boxes ──────────────────────────────────────────
	// Accumulate min/max canvas X for each city (only non-highway, non-building streets).
	struct FCityBounds { UCityDefinition* Def = nullptr; float MinX = MAX_FLT, MaxX = -MAX_FLT; };
	TMap<FName, FCityBounds> CityAccum;

	for (auto& KV : GridX)
	{
		UStreetDefinition** DefPtr = AllDefs.Find(KV.Key);
		if (!DefPtr || !*DefPtr) continue;
		UCityDefinition* City = (*DefPtr)->OwnerCity.Get();
		if (!City || (*DefPtr)->bIsHighway || (*DefPtr)->bIsPCGBuilding) continue;

		FCityBounds& B = CityAccum.FindOrAdd(City->CityID);
		B.Def  = City;
		B.MinX = FMath::Min(B.MinX, NodeTopLeft[KV.Key].X);
		B.MaxX = FMath::Max(B.MaxX, NodeTopLeft[KV.Key].X + NodeWidth);
	}

	// ── Create city backgrounds FIRST so they render behind street nodes ─────
	for (auto& KV : CityAccum)
	{
		if (!KV.Value.Def || KV.Value.MinX > KV.Value.MaxX) continue;

		const FVector2D BoxMin(KV.Value.MinX - CityPadX,
		                       CanvasOriginY  - CityPadTop);
		const FVector2D BoxMax(KV.Value.MaxX + CityPadX,
		                       CanvasOriginY  + NodeHeight + CityPadBot);

		CreateCityBackground(KV.Value.Def, BoxMin, BoxMax);

		// Store for NativePaint border outline.
		FCityRegion Region;
		Region.BorderColor = KV.Value.Def->MapColor;
		Region.BoxMin      = BoxMin;
		Region.BoxMax      = BoxMax;
		CityRegions.Add(Region);
	}

	// ── Create street node widgets ────────────────────────────────────────────
	for (auto& KV : GridX)
	{
		UStreetDefinition** DefPtr = AllDefs.Find(KV.Key);
		if (!DefPtr || !*DefPtr) continue;
		if ((*DefPtr)->bIsPCGBuilding) continue; // buildings are labels only

		const bool bVisited   = Visited.Contains(KV.Key);
		const bool bIsCurrent = (KV.Key == CurrentID);

		CreateNode(*DefPtr, NodeTopLeft[KV.Key], bVisited, bIsCurrent);
	}

	// ── Build connection line pairs ───────────────────────────────────────────
	for (auto& KV : GridX)
	{
		UStreetDefinition** DefPtr = AllDefs.Find(KV.Key);
		if (!DefPtr || !*DefPtr) continue;

		for (const FStreetExitLink& Exit : (*DefPtr)->Exits)
		{
			if (!Exit.Destination || Exit.Layout != EExitLayout::AdjacentRight) continue;
			if (!NodeCenters.Contains(KV.Key) || !NodeCenters.Contains(Exit.Destination->StreetID)) continue;

			const bool bHwy = (*DefPtr)->bIsHighway || Exit.Destination->bIsHighway;
			auto& Lines = bHwy ? HighwayLines : ConnectionLines;
			Lines.Add({ NodeCenters[KV.Key], NodeCenters[Exit.Destination->StreetID] });
		}
	}
}

void UMapWidget::CreateCityBackground(UCityDefinition* City, FVector2D BoxMin, FVector2D BoxMax)
{
	// Semi-transparent filled background
	UBorder* BgBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FLinearColor BgColor = City->MapColor;
	BgColor.A = 0.10f;
	BgBorder->SetBrushColor(BgColor);
	BgBorder->SetPadding(FMargin(0.f));

	UCanvasPanelSlot* BgSlot = MapCanvas->AddChildToCanvas(BgBorder);
	BgSlot->SetPosition(BoxMin);
	BgSlot->SetSize(BoxMax - BoxMin);
	BgSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
	BgSlot->SetZOrder(-1);

	// City name label above the region
	UTextBlock* CityLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CityLabel->SetText(City->CityName);
	FLinearColor LabelColor = City->MapColor;
	LabelColor.A = 0.85f;
	CityLabel->SetColorAndOpacity(FSlateColor(LabelColor));
	FSlateFontInfo LabelFont = CityLabel->GetFont();
	LabelFont.Size = 12;
	CityLabel->SetFont(LabelFont);

	UCanvasPanelSlot* LabelSlot = MapCanvas->AddChildToCanvas(CityLabel);
	LabelSlot->SetPosition(FVector2D(BoxMin.X + 6.f, BoxMin.Y + 4.f));
	LabelSlot->SetSize(FVector2D(BoxMax.X - BoxMin.X - 12.f, 20.f));
	LabelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
	LabelSlot->SetZOrder(-1);
}

void UMapWidget::CreateNode(UStreetDefinition* Street, FVector2D CanvasPos,
	bool bVisited, bool bIsCurrent)
{
	// Choose node background colour
	FLinearColor BgColor = bIsCurrent ? FLinearColor(0.50f, 0.38f, 0.02f, 1.f)   // gold
	                     : bVisited   ? FLinearColor(0.12f, 0.12f, 0.16f, 1.f)   // dark grey
	                     :             FLinearColor(0.05f, 0.05f, 0.07f, 0.55f); // fog

	// Highway nodes get a subtle amber tint when not current
	if (Street->bIsHighway && !bIsCurrent)
		BgColor = bVisited ? FLinearColor(0.18f, 0.14f, 0.04f, 1.f)
		                   : FLinearColor(0.08f, 0.06f, 0.01f, 0.55f);

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetBrushColor(BgColor);
	Border->SetPadding(FMargin(6.f, 4.f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Border->SetContent(VBox);

	// Street / highway name
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	const FText Label = Street->MapLabel.IsEmpty() ? Street->StreetName : Street->MapLabel;
	NameText->SetText(Label);
	FLinearColor NameColor = bVisited ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
	if (Street->bIsHighway && bVisited)
		NameColor = FLinearColor(0.9f, 0.75f, 0.3f, 1.f); // amber for highway text
	NameText->SetColorAndOpacity(FSlateColor(NameColor));
	FSlateFontInfo Font = NameText->GetFont();
	Font.Size = 11;
	NameText->SetFont(Font);
	VBox->AddChildToVerticalBox(NameText);

	// Building bullet labels (only when visited)
	if (bVisited)
	{
		for (const FStreetExitLink& Exit : Street->Exits)
		{
			if (Exit.Layout != EExitLayout::Building || !Exit.Destination) continue;

			UTextBlock* BldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			const FText BldLabel = Exit.Destination->MapLabel.IsEmpty()
				? Exit.Destination->StreetName : Exit.Destination->MapLabel;
			BldText->SetText(FText::Format(NSLOCTEXT("Map", "Bld", "  \u2022 {0}"), BldLabel));
			BldText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.85f, 1.f, 1.f)));
			FSlateFontInfo BldFont = BldText->GetFont();
			BldFont.Size = 9;
			BldText->SetFont(BldFont);
			VBox->AddChildToVerticalBox(BldText);
		}
	}

	UCanvasPanelSlot* PanelSlot = MapCanvas->AddChildToCanvas(Border);
	PanelSlot->SetPosition(CanvasPos);
	PanelSlot->SetSize(FVector2D(NodeWidth, NodeHeight));
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
}

int32 UMapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// City region border outlines
	for (const FCityRegion& R : CityRegions)
	{
		const FVector2D TL = R.BoxMin;
		const FVector2D TR(R.BoxMax.X, R.BoxMin.Y);
		const FVector2D BL(R.BoxMin.X, R.BoxMax.Y);
		const FVector2D BR = R.BoxMax;
		FLinearColor C = R.BorderColor;
		C.A = 0.45f;

		const FPaintGeometry PG = AllottedGeometry.ToPaintGeometry();
		auto DrawEdge = [&](FVector2D A, FVector2D B)
		{
			TArray<FVector2D> Pts = { A, B };
			FSlateDrawElement::MakeLines(OutDrawElements, Layer + 1, PG, Pts,
				ESlateDrawEffect::None, C, true, 1.5f);
		};
		DrawEdge(TL, TR); DrawEdge(TR, BR); DrawEdge(BR, BL); DrawEdge(BL, TL);
	}

	// Street-to-street connection lines (grey)
	for (const TPair<FVector2D, FVector2D>& Line : ConnectionLines)
	{
		TArray<FVector2D> Pts = { Line.Key, Line.Value };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer + 2,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, FLinearColor(0.45f, 0.45f, 0.45f, 1.f), true, 2.f);
	}

	// Highway connection lines (amber, slightly thicker)
	for (const TPair<FVector2D, FVector2D>& Line : HighwayLines)
	{
		TArray<FVector2D> Pts = { Line.Key, Line.Value };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer + 2,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, FLinearColor(0.90f, 0.68f, 0.10f, 1.f), true, 3.f);
	}

	return Layer + 2;
}

void UMapWidget::OnCloseClicked()
{
	ABaseCharacter* Player = Cast<ABaseCharacter>(GetOwningPlayerPawn());
	if (Player) Player->ToggleMap();
}
