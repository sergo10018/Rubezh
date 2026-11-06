// РУБЕЖ: Земля после разлома

#pragma once

#include "CoreMinimal.h"

#include "RubezhTraderKind.generated.h"

/** Торговец: у каждого вида предметов свой (столбец Trader в data/items.csv). */
UENUM(BlueprintType)
enum class ERubezhTraderKind : uint8
{
	None,
	Gunsmith,	// оружейник: оружие, патроны, магазины, чистка, ремонт
	Medic,		// медик: лекарства
	Armorer,	// бронник: броня, одежда, рюкзаки
	Scavenger	// скупщик: хлам, ценности, еда
};
