// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractionBehaviorComponent.h"
#include "Enemy/EnemyTypes.h"
#include "BreakableComponent.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBroken);

/**
 * Gives an AWorldProp a health pool that depletes when hit with the correct weapon.
 * When health reaches 0: spawns loot, fires OnBroken, then either destroys the actor
 * or swaps its mesh to BrokenMesh (when bDestroyOnDeath = false).
 *
 * Weapon damage reaches this component through AWorldProp::TakeMeleeDamage_Implementation.
 * RequiredTool gates which weapon type deals damage. Wrong tool = no damage.
 *
 * E-key interaction shows a hint ("Requires a Hammer") rather than dealing damage.
 *
 * Default Priority = 10 (higher than DisassembleComponent's 5).
 *
 * Blueprint steps: create BP_WoodPlanks as a child of AWorldProp, add BreakableComponent,
 * set ActionLabel, MaxHealth, RequiredTool, LootTable, and meshes in Details panel.
 */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TWODSURVIVAL_API UBreakableComponent : public UInteractionBehaviorComponent
{
	GENERATED_BODY()

public:
	UBreakableComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breakable", meta=(ClampMin="1.0"))
	float MaxHealth = 50.f;

	/** Items dropped when this prop is destroyed. Same format as enemy loot tables. */
	UPROPERTY(EditDefaultsOnly, Category="Breakable")
	TArray<FLootEntry> LootTable;

	/**
	 * If set, the owner's StaticMeshComponent swaps to this mesh on death (e.g. broken glass).
	 * Only used when bDestroyOnDeath = false.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Breakable")
	TObjectPtr<UStaticMesh> BrokenMesh;

	/** When true (default), the owner actor is destroyed when health reaches 0. */
	UPROPERTY(EditDefaultsOnly, Category="Breakable")
	bool bDestroyOnDeath = true;

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	TObjectPtr<USoundBase> SFX_Hit;

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	TObjectPtr<USoundBase> SFX_Destroyed;

	/** Fired when health reaches 0, after loot is spawned. Bind in Blueprint to unlock doors, etc. */
	UPROPERTY(BlueprintAssignable, Category="Breakable")
	FOnBroken OnBroken;

	UPROPERTY(BlueprintReadOnly, Category="Breakable")
	float CurrentHealth = 0.f;

	/**
	 * Called by AWorldProp::TakeMeleeDamage_Implementation.
	 * Checks RequiredTool, applies damage, and triggers death if health <= 0.
	 */
	void ApplyDamage(float Amount, ABaseCharacter* DamageSource);

	// UInteractionBehaviorComponent
	virtual FText GetPrompt() const override;
	virtual bool IsAvailable() const override;
	virtual bool CanInteract(ABaseCharacter* Interactor) const override;
	virtual void Execute(ABaseCharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsBroken = false;

	void OnHealthDepleted();
	void SpawnLoot();
};
