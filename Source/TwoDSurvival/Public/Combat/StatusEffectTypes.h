// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StatusEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EStatusEffect : uint8
{
	None        UMETA(DisplayName = "None"),
	Bleeding    UMETA(DisplayName = "Bleeding"),
	Infected    UMETA(DisplayName = "Infected"),
	Poisoned    UMETA(DisplayName = "Poisoned"),
	BrokenBone  UMETA(DisplayName = "Broken Bone"),
	Frostbite   UMETA(DisplayName = "Frostbite"),
	Hypothermia UMETA(DisplayName = "Hypothermia"),
	Wet         UMETA(DisplayName = "Wet"),
	Concussion  UMETA(DisplayName = "Concussion"),
};

/**
 * One active status effect on the player.
 * RemainingDuration == -1 means indefinite (removed only by cure items or internal logic).
 */
USTRUCT(BlueprintType)
struct FActiveStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EStatusEffect Type = EStatusEffect::None;

	UPROPERTY(BlueprintReadOnly)
	float Severity = 1.f;

	/** Seconds until this effect expires. -1 = indefinite. */
	UPROPERTY(BlueprintReadOnly)
	float RemainingDuration = -1.f;
};
