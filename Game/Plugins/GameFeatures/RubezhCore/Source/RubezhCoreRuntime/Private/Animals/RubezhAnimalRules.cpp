// РУБЕЖ: Земля после разлома

#include "Animals/RubezhAnimalRules.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhAnimalRules)

namespace Rubezh::Animals
{
	ERubezhAnimalState Decide(const FRubezhAnimalDefinition& Animal, const FSenses& Senses)
	{
		if (Senses.HealthFraction <= 0.0f)
		{
			return ERubezhAnimalState::Dead;
		}

		const bool bHurtEnoughToFlee = Senses.HealthFraction * 100.0f <= Animal.FleeHealthPercent;
		if (Senses.bStillFleeing || Senses.bPackRouted || bHurtEnoughToFlee)
		{
			return ERubezhAnimalState::Flee;
		}

		// В поселение звери не идут: цель за стенами для них потеряна.
		if (!Senses.bHasTarget || Senses.bTargetInSafeZone || Senses.TargetDistanceCm > Animal.LoseRadiusCm)
		{
			return ERubezhAnimalState::Wander;
		}
		return Senses.TargetDistanceCm <= Animal.BiteRangeCm ? ERubezhAnimalState::Bite : ERubezhAnimalState::Chase;
	}

	bool IsPackRouted(const FRubezhAnimalDefinition& Animal, int32 Alive, int32 Total)
	{
		if (Total <= 1 || Animal.PackRoutPercent <= 0.0f)
		{
			return false;
		}
		// В целых процентах: стая из 4 при 50 % бежит ровно после второй потери, без ошибки округления дроби.
		return (Total - Alive) * 100 >= FMath::RoundToInt(Animal.PackRoutPercent) * Total;
	}

	FVector2D GetSurroundOffset(int32 Index, int32 Count, float RadiusCm)
	{
		const double Angle = 2.0 * UE_DOUBLE_PI * Index / FMath::Max(Count, 1);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RadiusCm;
	}

	float GetChaseSpeed(const FRubezhAnimalDefinition& Animal, float TargetDistanceCm)
	{
		const bool bCharging = Animal.ChargeSpeed > 0.0f && TargetDistanceCm <= Animal.ChargeStartCm;
		return bCharging ? Animal.ChargeSpeed : Animal.RunSpeed;
	}
}
