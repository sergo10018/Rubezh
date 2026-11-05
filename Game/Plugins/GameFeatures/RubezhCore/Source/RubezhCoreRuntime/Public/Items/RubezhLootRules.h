// РУБЕЖ: Земля после разлома
// Правила лута: таблицы лута ящиков, перенос вещей между рюкзаком и ящиком, что остаётся на теле.
// Только данные и математика — проверяется тестами. Таблицы — data/loot_tables.csv.

#pragma once

#include "Engine/DataTable.h"
#include "Items/RubezhInventoryGrid.h"
#include "Math/RandomStream.h"

#include "RubezhLootRules.generated.h"

/**
 * Строка таблицы лута (DataTable из data/loot_tables.csv): с каким шансом и сколько штук предмета
 * кладётся в ящик этого типа. Имя строки ни на что не влияет — важен столбец Table.
 */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// Тип места: military, medical, civil ... (его задают у ящика в поле Loot Table).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot")
	FName Table;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot")
	FName ItemId;

	// Шанс появления, %.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot", Meta = (ClampMin = 0, ClampMax = 100))
	float Chance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot", Meta = (ClampMin = 1))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot", Meta = (ClampMin = 1))
	int32 MaxCount = 1;
};

namespace Rubezh::Loot
{
	struct FDrop
	{
		FName ItemId;
		int32 Count = 0;
	};

	/** Что выпало в ящике типа Table: каждая строка таблицы проверяется отдельно по своему шансу. */
	RUBEZHCORERUNTIME_API TArray<FDrop> Roll(TConstArrayView<FRubezhLootTableRow> Rows, FName Table, FRandomStream& Random);

	/**
	 * Переложить стопку из одного рюкзака/ящика в другой: в пустую ячейку TargetSlot — целиком, на такой же предмет —
	 * сколько влезет в его стопку; если ячейка не задана или занята другим предметом — куда влезет.
	 * Возвращает, сколько штук переложено; не влезшее остаётся на месте.
	 */
	RUBEZHCORERUNTIME_API int32 Transfer(FRubezhInventoryGrid& From, int32 EntryIndex, FRubezhInventoryGrid& To,
		TOptional<int32> TargetSlot, FRubezhItemLookup Lookup);

	/**
	 * Вынуть из рюкзака всё, что теряется при смерти (docs/gdd/00_Core_loop.md §4: расходники, хлам, ценности),
	 * и вернуть отдельным набором ячеек — это ляжет на тело. Экипировка и неизвестные предметы остаются у игрока.
	 */
	RUBEZHCORERUNTIME_API FRubezhInventoryGrid TakeLostOnDeath(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup);
}
