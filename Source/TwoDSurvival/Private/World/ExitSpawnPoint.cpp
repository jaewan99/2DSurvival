// Fill out your copyright notice in the Description page of Project Settings.

#include "World/ExitSpawnPoint.h"

#if WITH_EDITOR
#include "Components/ArrowComponent.h"
#endif

AExitSpawnPoint::AExitSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	// Large blue arrow so the spawn point is easy to see and orient in the editor.
	EditorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(RootComponent);
		EditorArrow->ArrowColor    = FColor(0, 128, 255);
		EditorArrow->ArrowSize     = 2.f;
		EditorArrow->bIsScreenSizeScaled = true;
		EditorArrow->SetHiddenInGame(true);
	}
#endif
}
