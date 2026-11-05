// РУБЕЖ: Земля после разлома

#include "Survival/RubezhStaminaRules.h"

namespace Rubezh::Stamina
{
	bool CanStartSprint(float Stamina, ERubezhEncumbranceLevel Level)
	{
		return Stamina >= MinStaminaToStartSprint && Encumbrance::GetEffects(Level).bCanSprint;
	}

	float GetSprintNetDrainPerSecond(ERubezhEncumbranceLevel Level)
	{
		return SprintNetDrainPerSecond * Encumbrance::GetEffects(Level).StaminaCostMultiplier;
	}

	float GetSprintDrainPerTick(ERubezhEncumbranceLevel Level)
	{
		return -(GetSprintNetDrainPerSecond(Level) + RegenPerSecond) * TickPeriod;
	}
}
