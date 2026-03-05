// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetBootstrapper.generated.h"

class UStreetDefinition;

/**
 * Place one of these in the persistent level (the empty level that always stays loaded).
 * On BeginPlay it tells UStreetManager which street to load first and at what world offset.
 *
 * Blueprint child (BP_StreetBootstrapper):
 *   - Assign StartingStreet (DA_Street_* data asset) in Details
 *   - Leave StartingOffset at zero unless the first street shouldn't start at the world origin
 */
UCLASS()
class TWODSURVIVAL_API AStreetBootstrapper : public AActor
{
	GENERATED_BODY()

public:
	// The street to stream in when the game starts.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	TObjectPtr<UStreetDefinition> StartingStreet;

	// World-space X offset for the starting street. Usually leave at zero.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street")
	FVector StartingOffset = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;
};
