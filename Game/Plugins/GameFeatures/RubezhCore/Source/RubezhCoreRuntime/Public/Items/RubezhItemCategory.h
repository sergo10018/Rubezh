// РУБЕЖ: Земля после разлома

#pragma once

#include "CoreMinimal.h"

#include "RubezhItemCategory.generated.h"

/**
 * Категория предмета. Задаётся в данных предмета и решает, теряется ли он при смерти
 * (docs/gdd/00_Core_loop.md §4).
 */
UENUM(BlueprintType)
enum class ERubezhItemCategory : uint8
{
	Equipment,	// оружие, броня, разгрузка, рюкзак, одежда — надетые или в рюкзаке
	Consumable,	// патроны, медикаменты, еда, вода, наборы для чистки
	Junk,		// найденный хлам: детали, лом, бытовые предметы
	Valuable	// добытые ресурсы и ценности: материалы, документы, трофеи
};

namespace Rubezh::Items
{
	/** Остаётся ли предмет на теле при смерти. Экипировка сохраняется, остальное — ставка вылазки. */
	RUBEZHCORERUNTIME_API bool IsLostOnDeath(ERubezhItemCategory Category);
}
