// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class USphereComponent;

/**
 * Add this component to ABaseCharacter (or any ACharacter) to give it proximity-based interaction.
 *
 * - Creates a detection sphere at runtime (radius = DetectionRadius).
 * - Tracks all nearby actors that implement IInteractable; focuses the closest one.
 * - StartInteract(): instant interactions fire immediately; hold interactions lock movement
 *   and fill InteractionProgress until the hold duration is met.
 * - StopInteract(): cancels an in-progress hold and restores movement.
 *
 * Bind the E key to StartInteract / StopInteract in Blueprint or via Input Mappings.
 * Bind InteractionProgress and InteractionPrompt to a UMG progress-bar widget.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	// Radius of the proximity detection sphere (world units).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float DetectionRadius = 150.f;

	// Filled 0→1 while holding a Hold-type interactable. Bind to a UMG progress bar.
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	float InteractionProgress = 0.f;

	// Prompt text of the currently focused interactable (e.g. "Open Door"). Bind to UMG.
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText InteractionPrompt;

	// True while a Hold interaction is actively in progress.
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsInteracting = false;

	/** Call on interaction key pressed. Fires instant interactions immediately; starts hold timer. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StartInteract();

	/** Call on interaction key released. Cancels an in-progress hold interaction. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopInteract();

	/** Returns true when an interactable is in range and focused. Useful for showing/hiding the prompt UI. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool HasFocusedInteractable() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	USphereComponent* DetectionSphere;

	// All actors with IInteractable currently inside the detection sphere.
	UPROPERTY()
	TArray<AActor*> NearbyInteractables;

	// The closest interactable — the one StartInteract acts on.
	UPROPERTY()
	AActor* FocusedInteractable;

	float HoldElapsed = 0.f;

	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Recalculates FocusedInteractable from NearbyInteractables (picks closest).
	void UpdateFocusedInteractable();

	void CompleteInteraction();
	void CancelInteraction();
};
