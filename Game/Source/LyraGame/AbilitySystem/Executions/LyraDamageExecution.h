// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "LyraDamageExecution.generated.h"

class UObject;
class UAbilitySystemComponent;
struct FHitResult;

/**
 * Hook for game feature plugins: returns a multiplier for damage the target receives (e.g. armor).
 * Rubezh: bound by RubezhCoreRuntime (equipment protection).
 */
DECLARE_DELEGATE_RetVal_TwoParams(float, FLyraIncomingDamageMultiplier, const UAbilitySystemComponent* /*Target*/, const FHitResult* /*Hit*/);


/**
 * ULyraDamageExecution
 *
 *	Execution used by gameplay effects to apply damage to the health attributes.
 */
UCLASS()
class ULyraDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:

	ULyraDamageExecution();

	/** Rubezh: optional target-side damage multiplier (armor). Unbound means 1. */
	static LYRAGAME_API FLyraIncomingDamageMultiplier& GetIncomingDamageMultiplier();

protected:

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
