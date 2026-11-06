// РУБЕЖ: Земля после разлома
// Правила торговли: цены, покупка, продажа, ремонт и чистка оружия (docs/gdd/01_Системы_прототипа.md §4).
// Только данные и математика — проверяется тестами. Вызывает сервер; клиент — только чтобы сразу объяснить отказ.

#pragma once

#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhTraderKind.h"
#include "Items/RubezhWeaponRules.h"

namespace Rubezh::Trade
{
	/** Какую долю цены торговец платит за предмет. Меньше 1: купить и сразу продать всегда убыточно. */
	inline constexpr float SellPriceFraction = 0.5f;

	/** Полный ремонт оружия с нулевой прочности стоит столько процентов его цены. Целое: дробь 0,3 округлила бы цену вверх на билет. */
	inline constexpr int32 RepairPricePercent = 30;

	/** Чистка в поле не поднимает прочность выше этого; дальше — только ремонт в поселении. */
	inline constexpr float CleanDurabilityCap = 70.0f;

	enum class EResult : uint8
	{
		Ok,
		NotForSale,			// этот торговец такое не продаёт
		NotBought,			// этот торговец такое не покупает
		NotEnoughMoney,
		NoRoom,				// в рюкзаке нет свободной ячейки
		Missing,			// нет такой стопки
		NothingToRepair
	};

	RUBEZHCORERUNTIME_API const TCHAR* Describe(EResult Result);
	RUBEZHCORERUNTIME_API const TCHAR* GetTraderName(ERubezhTraderKind Kind);

	/** Как называются деньги, в единственном месте кода. */
	RUBEZHCORERUNTIME_API FString FormatMoney(int32 Amount);

	RUBEZHCORERUNTIME_API bool IsSoldBy(const FRubezhItemDefinition& Item, ERubezhTraderKind Kind);
	RUBEZHCORERUNTIME_API bool IsBoughtBy(const FRubezhItemDefinition& Item, ERubezhTraderKind Kind);

	/** Сколько торговец заплатит за стопку целиком: с учётом износа, вставленного магазина и патронов внутри. */
	RUBEZHCORERUNTIME_API int32 GetSellValue(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Item,
		FRubezhItemLookup Items, Weapons::FWeaponLookup WeaponStats);

	/** Сколько стоит починить до полной прочности; 0 — чинить нечего. */
	RUBEZHCORERUNTIME_API int32 GetRepairCost(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Item);

	/** Купить Count штук: либо всё сразу, либо ничего. */
	RUBEZHCORERUNTIME_API EResult Buy(FRubezhInventoryGrid& Grid, int32& Money, FName ItemId, int32 Count, ERubezhTraderKind Kind, FRubezhItemLookup Items);

	/** Продать стопку целиком. */
	RUBEZHCORERUNTIME_API EResult Sell(FRubezhInventoryGrid& Grid, int32& Money, int32 EntryIndex, ERubezhTraderKind Kind,
		FRubezhItemLookup Items, Weapons::FWeaponLookup WeaponStats);

	/** Починить до полной прочности. Осечку следующего выстрела после этого надо бросить заново. */
	RUBEZHCORERUNTIME_API EResult Repair(FRubezhInventoryEntry& Entry, int32& Money, const FRubezhItemDefinition& Item);

	RUBEZHCORERUNTIME_API bool CanClean(float Durability);

	/** Прочность после чистки: прибавка, но не выше CleanDurabilityCap и не ниже текущей. */
	RUBEZHCORERUNTIME_API float GetCleanedDurability(float Durability, float CleanPercent);
}
