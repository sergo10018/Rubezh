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

	// Смерть (docs/gdd/00_Core_loop.md §4): шанс выпадения. Для стопки — свой бросок на каждую, для патронов — один на смерть.
	inline constexpr float DeathDropChancePercent = 30.0f;
	// Какая доля патронов выпадает, если бросок удался.
	inline constexpr float DeathAmmoLossMinPercent = 5.0f;
	inline constexpr float DeathAmmoLossMaxPercent = 7.0f;

	/** Сколько патронов выпадает из Total при доле Percent, с обычным округлением: из маленького запаса может не выпасть ни одного. */
	RUBEZHCORERUNTIME_API int32 ComputeAmmoLoss(int32 Total, float Percent);

	/**
	 * Вынуть из рюкзака Percent % патронов каждого калибра, считая и лежащие в магазинах: сначала россыпью,
	 * потом из наименее полных магазинов. Сами магазины и патроны внутри оружия остаются. Возвращает вынутое (без нулей).
	 */
	RUBEZHCORERUNTIME_API TArray<FDrop> TakeAmmoLoss(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup, float Percent);

	/**
	 * Вынуть из рюкзака то, что выпало при смерти (docs/gdd/00_Core_loop.md §4), и вернуть отдельным набором ячеек —
	 * это ляжет на тело. Каждая стопка расходников, хлама и ценностей уходит целиком с шансом DeathDropChancePercent;
	 * с тем же шансом, одним броском на смерть, выпадает часть патронов. Экипировка, магазины и неизвестные предметы остаются.
	 */
	RUBEZHCORERUNTIME_API FRubezhInventoryGrid TakeLostOnDeath(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup, FRandomStream& Random);
}
