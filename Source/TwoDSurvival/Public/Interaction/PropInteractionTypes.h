// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PropInteractionTypes.generated.h"

/** Tool type required to interact with or break a prop. */
UENUM(BlueprintType)
enum class EToolType : uint8
{
	None     UMETA(DisplayName = "None (Any)"),
	Hammer   UMETA(DisplayName = "Hammer"),
	Axe      UMETA(DisplayName = "Axe"),
	Crowbar  UMETA(DisplayName = "Crowbar"),
};
