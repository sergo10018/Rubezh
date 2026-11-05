// РУБЕЖ: Земля после разлома
// Доступ к таблицам предметов и лута, заданным в настройках проекта (URubezhSettings).

#pragma once

#include "Items/RubezhLootRules.h"

struct FRubezhItemDefinition;

namespace Rubezh::Items
{
	/** Описание предмета из DT_Items; nullptr — предмет неизвестен или таблица не задана. */
	RUBEZHCORERUNTIME_API const FRubezhItemDefinition* FindDefinition(FName ItemId);

	/** Все строки таблицы лута (DT_LootTables); пусто — таблица не задана. */
	RUBEZHCORERUNTIME_API TArray<FRubezhLootTableRow> GetLootRows();
}
