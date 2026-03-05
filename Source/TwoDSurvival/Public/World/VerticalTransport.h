// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "VerticalTransport.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class ABaseCharacter;

UENUM(BlueprintType)
enum class ETransportType : uint8
{
	Stairs  UMETA(DisplayName = "Stairs"),
	Ladder  UMETA(DisplayName = "Ladder"),
};

/**
 * Handles vertical movement between floors inside buildings.
 * Press E from the lower floor → teleports to the upper floor.
 * Press E from the upper floor → teleports to the lower floor.
 *
 * Used for both stairs and ladders — set TransportType for the correct prompt.
 * ABuildingGenerator spawns this at each stair/ladder column (except the top floor).
 *
 * Blueprint children:
 *   BP_Staircase — assign a stair mesh
 *   BP_Ladder    — assign a ladder mesh
 */
UCLASS()
class TWODSURVIVAL_API AVerticalTransport : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AVerticalTransport();

	// Root — stays at the actor's spawn position so children offset correctly.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	// Assign the stair/ladder mesh in the Blueprint Details panel.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Physical collision — blocks the player from passing through the stair structure.
	// Child of Mesh, sized to match the cell (RoomWidth × FloorHeight).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	// Overlap-only trigger for the E-press interaction. Spans both floor levels.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	// Determines the interaction prompt text ("Go Upstairs/Downstairs" vs "Climb Up/Down").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transport")
	ETransportType TransportType = ETransportType::Stairs;

	// Vertical distance to the next floor. Set automatically by ABuildingGenerator;
	// override manually if placing this actor by hand.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transport")
	float FloorHeight = 400.f;

	// Updates FloorHeight and resizes the InteractionBox to span the full floor gap.
	// Called by ABuildingGenerator after spawning so the box covers both floor levels.
	void SetFloorHeight(float NewFloorHeight);

	// IInteractable
	virtual EInteractionType GetInteractionType_Implementation() override { return EInteractionType::Instant; }
	virtual float GetInteractionDuration_Implementation() override { return 0.f; }
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	// Cached when the player enters the overlap zone — used to show a directional prompt.
	TWeakObjectPtr<ABaseCharacter> NearbyPlayer;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// True if the character is standing on the lower floor (below the midpoint).
	bool IsPlayerOnLowerFloor(ABaseCharacter* Player) const;

	// Resizes both boxes to match the current FloorHeight.
	void ResizeBoxes();
};
