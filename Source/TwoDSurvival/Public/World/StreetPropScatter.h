// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetPropScatter.generated.h"

class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;

// ── One entry in the prop pool ────────────────────────────────────────────────

/**
 * One mesh variant in a scatter pool.
 *
 * Weight is relative — if TreeA has Weight=2 and LeafPile has Weight=1,
 * TreeA appears roughly twice as often.
 */
USTRUCT(BlueprintType)
struct FPropScatterEntry
{
	GENERATED_BODY()

	/** The static mesh to place. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> Mesh;

	/**
	 * Relative spawn weight. Higher = appears more often.
	 * All weights in the pool are summed; each entry's share = Weight / Total.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float Weight = 1.f;

	/** Minimum uniform scale applied to the spawned mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float ScaleMin = 0.8f;

	/** Maximum uniform scale applied to the spawned mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float ScaleMax = 1.2f;
};

// ── Scatter actor ─────────────────────────────────────────────────────────────

/**
 * Place in a street sublevel to randomly scatter decorative props (trees, leaf piles,
 * branches, trash, etc.) within a defined zone at runtime.
 *
 * The actor's Y position is used for all spawned meshes — props stay on the same
 * depth plane as the scatter actor, consistent with the 2D side-scroll view.
 *
 * Blueprint child BP_StreetPropScatter:
 *   - Resize ScatterZone to cover the street area in the viewport.
 *   - Fill PropPool with meshes and tweak weights/scale ranges.
 *   - Set MinCount / MaxCount for density.
 */
UCLASS()
class TWODSURVIVAL_API AStreetPropScatter : public AActor
{
	GENERATED_BODY()

public:
	AStreetPropScatter();

	/** Defines the XZ area in which props are scattered. Y is ignored — props use the actor's Y. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ScatterZone;

	/** Meshes to choose from when spawning each prop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scatter")
	TArray<FPropScatterEntry> PropPool;

	/** Minimum number of props to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0"))
	int32 MinCount = 3;

	/** Maximum number of props to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0"))
	int32 MaxCount = 8;

protected:
	virtual void BeginPlay() override;

private:
	/** Picks a random entry from PropPool using weighted selection. Returns nullptr if pool is empty. */
	const FPropScatterEntry* PickWeightedEntry() const;
};
