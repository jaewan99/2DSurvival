// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PauseMenuWidget.h"
#include "Character/BaseCharacter.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Widget must tick and receive input even while the game is paused.
	SetTickableWhenPaused(true);

	if (ResumeButton) ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	if (SaveButton)   SaveButton->OnClicked.AddDynamic(this,   &UPauseMenuWidget::OnSaveClicked);
	if (LoadButton)   LoadButton->OnClicked.AddDynamic(this,   &UPauseMenuWidget::OnLoadClicked);
	if (QuitButton)   QuitButton->OnClicked.AddDynamic(this,   &UPauseMenuWidget::OnQuitClicked);
}

void UPauseMenuWidget::SetOwnerCharacter(ABaseCharacter* InCharacter)
{
	OwnerCharacter = InCharacter;
}

void UPauseMenuWidget::OnResumeClicked()
{
	if (OwnerCharacter) OwnerCharacter->TogglePauseMenu();
}

void UPauseMenuWidget::OnSaveClicked()
{
	if (OwnerCharacter) OwnerCharacter->SaveGame();
}

void UPauseMenuWidget::OnLoadClicked()
{
	if (!OwnerCharacter) return;

	// Load first, then close the menu and unpause.
	OwnerCharacter->LoadGame();
	OwnerCharacter->TogglePauseMenu();
}

void UPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
