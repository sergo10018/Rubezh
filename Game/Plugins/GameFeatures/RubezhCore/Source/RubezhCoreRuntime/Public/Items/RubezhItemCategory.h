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
	Equipment,	// оружие, магазины, броня, разгрузка, рюкзак, одежда — надетые или в рюкзаке
	Consumable,	// медикаменты, еда, вода, наборы для чистки
	Junk,		// найденный хлам: детали, лом, бытовые предметы
	Valuable,	// добытые ресурсы и ценности: материалы, документы, трофеи
	Ammo		// патроны: при смерти выпадает только часть (Rubezh::Loot::TakeLostOnDeath)
};

namespace Rubezh::Items
{
	/** Остаётся ли на теле вся стопка при смерти. Экипировка сохраняется, патроны теряются частично, остальное — ставка вылазки. */
	RUBEZHCORERUNTIME_API bool IsLostOnDeath(ERubezhItemCategory Category);
}
