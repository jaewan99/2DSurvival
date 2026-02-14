// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractionTypes.generated.h"

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	Instant UMETA(DisplayName = "Instant"),
	Hold    UMETA(DisplayName = "Hold"),
};
