// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StreetManager.h"
#include "Engine/LevelStreamingDynamic.h"

void UStreetManager::InitializeWithStreet(UStreetDefinition* StartStreet, FVector WorldOffset)
{
	if (!StartStreet) return;

	PendingStreet  = StartStreet;
	PendingOffset  = WorldOffset;
	StreamingToUnload = nullptr;

	LoadStreet(StartStreet, WorldOffset);
}

void UStreetManager::OnPlayerCrossedExit(EExitDirection Direction)
{
	if (bTransitionInProgress) return;
	if (!CurrentStreet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] OnPlayerCrossedExit: CurrentStreet not set yet."));
		return;
	}

	UStreetDefinition* NextStreet = CurrentStreet->GetExit(Direction);
	if (!NextStreet)
	{
		UE_LOG(LogTemp, Log, TEXT("[StreetManager] Exit %d is blocked — no adjacent street."), (int32)Direction);
		return;
	}

	const FVector NextOffset = ComputeAdjacentOffset(Direction, NextStreet);

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Player crossed %d — streaming '%s' at offset X=%.0f."),
		(int32)Direction, *NextStreet->StreetID.ToString(), NextOffset.X);

	bTransitionInProgress = true;
	StreamingToUnload = ActiveStreaming;
	PendingStreet     = NextStreet;
	PendingOffset     = NextOffset;

	LoadStreet(NextStreet, NextOffset);
}

void UStreetManager::LoadStreet(UStreetDefinition* Street, FVector WorldOffset)
{
	if (!Street || Street->Level.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] LoadStreet: '%s' has no Level asset assigned."),
			Street ? *Street->StreetID.ToString() : TEXT("null"));
		bTransitionInProgress = false;
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	bool bSuccess = false;
	const FTransform LevelTransform(FQuat::Identity, WorldOffset, FVector::OneVector);

	ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		World, Street->Level, LevelTransform, bSuccess);

	if (!Streaming || !bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[StreetManager] LoadLevelInstanceBySoftObjectPtr failed for '%s'."),
			*Street->StreetID.ToString());
		bTransitionInProgress = false;
		return;
	}

	PendingStreaming = Streaming;
	Streaming->OnLevelShown.AddDynamic(this, &UStreetManager::OnNewStreetShown);
}

void UStreetManager::OnNewStreetShown()
{
	// Unbind immediately — this delegate fires once per load
	if (PendingStreaming)
	{
		PendingStreaming->OnLevelShown.RemoveDynamic(this, &UStreetManager::OnNewStreetShown);
	}

	// Unload the previous street
	if (StreamingToUnload)
	{
		StreamingToUnload->SetShouldBeLoaded(false);
		StreamingToUnload->SetShouldBeVisible(false);
		StreamingToUnload = nullptr;
	}

	// Commit the new street as current
	ActiveStreaming           = PendingStreaming;
	CurrentStreet             = PendingStreet;
	CurrentStreetWorldOffset  = PendingOffset;

	PendingStreaming = nullptr;
	PendingStreet   = nullptr;
	bTransitionInProgress = false;

	UE_LOG(LogTemp, Log, TEXT("[StreetManager] Now in '%s' at X=%.0f."),
		*CurrentStreet->StreetID.ToString(), CurrentStreetWorldOffset.X);

	OnStreetChanged.Broadcast();
}

FVector UStreetManager::ComputeAdjacentOffset(EExitDirection Direction, UStreetDefinition* AdjacentStreet) const
{
	FVector Offset = CurrentStreetWorldOffset;

	switch (Direction)
	{
	case EExitDirection::Right:
		// Adjacent street starts where the current one ends
		Offset.X += CurrentStreet->StreetWidth;
		break;

	case EExitDirection::Left:
		// Adjacent street ends where the current one starts
		Offset.X -= AdjacentStreet->StreetWidth;
		break;

	case EExitDirection::Up:
		// Up transitions are intentional gates handled by a separate interactable.
		// UStreetManager does not handle seamless streaming for Up yet.
		UE_LOG(LogTemp, Warning, TEXT("[StreetManager] Up direction streaming not yet implemented."));
		break;
	}

	return Offset;
}
