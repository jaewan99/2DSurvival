// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class ABaseCharacter;

/**
 * Pause menu — shown when Escape is pressed (and not in placement mode).
 * Provides Resume, Save, Load, and Quit buttons.
 *
 * Blueprint child (WBP_PauseMenu) only needs:
 *   - Four Buttons named ResumeButton, SaveButton, LoadButton, QuitButton
 * Zero Blueprint logic required.
 */
UCLASS()
class TWODSURVIVAL_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by ABaseCharacter after creating this widget. */
	void SetOwnerCharacter(ABaseCharacter* InCharacter);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LoadButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

private:
	UPROPERTY()
	TObjectPtr<ABaseCharacter> OwnerCharacter;

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnSaveClicked();

	UFUNCTION()
	void OnLoadClicked();

	UFUNCTION()
	void OnQuitClicked();
};
