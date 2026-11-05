// РУБЕЖ: Земля после разлома

#pragma once

#include "CoreMinimal.h"

#include "RubezhInjury.generated.h"

/** Травма (docs/gdd/01_Системы_прототипа.md §3). В данных предмета — что он лечит (столбец Cures в data/items.csv). */
UENUM(BlueprintType)
enum class ERubezhInjury : uint8
{
	None,
	Bleeding,		// −1 HP/с, лечится бинтом
	HeavyBleeding,	// −3 HP/с, лечится жгутом
	LegFracture,	// нельзя бегать, лечится шиной
	Pain			// дрожит прицел; обезболивающее снимает на время
};

/** Набор травм одним числом: бит на каждую травму. */
namespace Rubezh::Injuries
{
	using FMask = uint8;

	constexpr FMask Bit(ERubezhInjury Injury)
	{
		return Injury == ERubezhInjury::None ? 0 : static_cast<FMask>(1u << (static_cast<uint8>(Injury) - 1));
	}

	constexpr bool Has(FMask Mask, ERubezhInjury Injury)
	{
		return (Mask & Bit(Injury)) != 0;
	}
}
