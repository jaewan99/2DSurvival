// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DecorativeProp.generated.h"

class UStaticMeshComponent;

/**
 * A purely visual prop with no interaction (chair, table, hospital bed, bar stool, etc.)
 *
 * Blueprint children: BP_Chair, BP_Table, BP_HospitalBed, BP_CounterDesk, BP_BarStool, etc.
 *   - Assign a Static Mesh to the Mesh component in the Details panel.
 *   - Set SpawnProbability (0–1) to control how often this prop appears (default: 1 = always).
 *   - Populate VariantMeshes with alternative meshes (different paints, materials, shapes).
 *     On BeginPlay one variant is chosen at random. Leave empty to always use the default mesh.
 */
UCLASS()
class TWODSURVIVAL_API ADecorativeProp : public AActor
{
	GENERATED_BODY()

public:
	ADecorativeProp();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// ── Probability ───────────────────────────────────────────────────────────

	/**
	 * Chance (0–1) that this prop actually appears when the room is loaded.
	 * 1.0 = always spawns (default). 0.5 = 50% chance. 0.0 = never spawns.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnProbability = 1.f;

	// ── Variety ───────────────────────────────────────────────────────────────

	/**
	 * Optional alternative meshes for visual variety (e.g. different paint colours,
	 * worn vs. clean variants, alternate shapes). Leave empty to always use the
	 * default mesh set in the Blueprint. When populated, BeginPlay picks one at
	 * random (including the possibility of the base mesh if you include it here).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop")
	TArray<TSoftObjectPtr<UStaticMesh>> VariantMeshes;

protected:
	virtual void BeginPlay() override;
};
