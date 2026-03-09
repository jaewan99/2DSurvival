// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetDefinition.h"

const FStreetExitLink* UStreetDefinition::GetExit(FName ExitID) const
{
	for (const FStreetExitLink& Link : Exits)
	{
		if (Link.ExitID == ExitID) return &Link;
	}
	return nullptr;
}
