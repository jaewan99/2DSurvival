// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetDefinition.h"

UStreetDefinition* UStreetDefinition::GetExit(EExitDirection Direction) const
{
	switch (Direction)
	{
	case EExitDirection::Left:  return ExitLeft;
	case EExitDirection::Right: return ExitRight;
	case EExitDirection::Up:    return ExitUp;
	default:                    return nullptr;
	}
}
