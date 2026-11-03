// РУБЕЖ: Земля после разлома

#include "Survival/RubezhEncumbrance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhEncumbrance)

namespace Rubezh::Encumbrance
{
	ERubezhEncumbranceLevel GetLevel(float CarriedWeight, float MaxCarryWeight)
	{
		if (CarriedWeight <= 0.0f)
		{
			return ERubezhEncumbranceLevel::Normal;
		}

		// Без лимита (например, атрибут ещё не инициализирован) любой вес — неподъёмный.
		if (MaxCarryWeight <= 0.0f)
		{
			return ERubezhEncumbranceLevel::Immobile;
		}

		const float Ratio = CarriedWeight / MaxCarryWeight;
		if (Ratio <= BurdenedRatio)
		{
			return ERubezhEncumbranceLevel::Normal;
		}
		if (Ratio <= OverloadedRatio)
		{
			return ERubezhEncumbranceLevel::Burdened;
		}
		if (Ratio <= ImmobileRatio)
		{
			return ERubezhEncumbranceLevel::Overloaded;
		}
		return ERubezhEncumbranceLevel::Immobile;
	}

	FRubezhEncumbranceEffects GetEffects(ERubezhEncumbranceLevel Level)
	{
		FRubezhEncumbranceEffects Effects;
		switch (Level)
		{
		case ERubezhEncumbranceLevel::Normal:
			break;
		case ERubezhEncumbranceLevel::Burdened:
			Effects.MoveSpeedMultiplier = 0.85f;
			Effects.StaminaCostMultiplier = 1.3f;
			break;
		case ERubezhEncumbranceLevel::Overloaded:
			Effects.MoveSpeedMultiplier = 0.65f;
			Effects.StaminaCostMultiplier = 2.0f;
			Effects.bCanSprint = false;
			break;
		case ERubezhEncumbranceLevel::Immobile:
			Effects.MoveSpeedMultiplier = 0.0f;
			Effects.StaminaCostMultiplier = 2.0f;
			Effects.bCanSprint = false;
			Effects.bCanMove = false;
			break;
		}
		return Effects;
	}
}
