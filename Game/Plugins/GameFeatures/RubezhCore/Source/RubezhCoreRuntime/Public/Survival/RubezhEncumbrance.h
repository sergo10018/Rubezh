// РУБЕЖ: Земля после разлома

#pragma once

#include "CoreMinimal.h"

#include "RubezhEncumbrance.generated.h"

/** Уровень нагрузки от доли переносимого веса (docs/gdd/01_Системы_прототипа.md §2). */
UENUM(BlueprintType)
enum class ERubezhEncumbranceLevel : uint8
{
	Normal,		// до 60 % лимита
	Burdened,	// 60–100 %
	Overloaded,	// 100–130 %
	Immobile	// больше 130 %: только сбросить вес
};

/** Что нагрузка делает с персонажем. */
USTRUCT(BlueprintType)
struct FRubezhEncumbranceEffects
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Survival")
	float MoveSpeedMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Survival")
	float StaminaCostMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Survival")
	bool bCanSprint = true;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Survival")
	bool bCanMove = true;
};

namespace Rubezh::Encumbrance
{
	inline constexpr float BurdenedRatio = 0.6f;
	inline constexpr float OverloadedRatio = 1.0f;
	inline constexpr float ImmobileRatio = 1.3f;

	/** Базовый лимит веса персонажа, кг. */
	inline constexpr float BaseMaxCarryWeight = 30.0f;

	RUBEZHCORERUNTIME_API ERubezhEncumbranceLevel GetLevel(float CarriedWeight, float MaxCarryWeight);

	RUBEZHCORERUNTIME_API FRubezhEncumbranceEffects GetEffects(ERubezhEncumbranceLevel Level);
}
