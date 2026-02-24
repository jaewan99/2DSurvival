// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HealthTypes.generated.h"

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
	Head     UMETA(DisplayName = "Head"),
	Body     UMETA(DisplayName = "Body"),
	LeftArm  UMETA(DisplayName = "Left Arm"),
	RightArm UMETA(DisplayName = "Right Arm"),
	LeftLeg  UMETA(DisplayName = "Left Leg"),
	RightLeg UMETA(DisplayName = "Right Leg")
};

USTRUCT(BlueprintType)
struct FBodyPartHealth
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHealth = 100.f;

	bool IsBroken() const { return CurrentHealth <= 0.f; }
};
