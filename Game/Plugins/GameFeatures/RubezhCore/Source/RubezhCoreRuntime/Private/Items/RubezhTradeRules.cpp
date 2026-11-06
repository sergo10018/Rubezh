// РУБЕЖ: Земля после разлома

#include "Items/RubezhTradeRules.h"

#include "Items/RubezhEquipment.h"

namespace Rubezh::Trade
{
	namespace
	{
		int64 GetLoadedValueBeforeRounding(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Item,
			FRubezhItemLookup Items, Weapons::FWeaponLookup WeaponStats)
		{
			int64 Price = 0;
			if (Entry.Rounds > 0 && !Item.AmmoItem.IsNone())
			{
				const FRubezhItemDefinition* Ammo = Items(Item.AmmoItem);
				Price += Ammo ? static_cast<int64>(Ammo->Price) * Entry.Rounds : 0;
			}
			if (Entry.bHasMagazine)
			{
				const FRubezhWeaponDefinition* Weapon = WeaponStats(Entry.ItemId);
				const FRubezhItemDefinition* Magazine = Weapon ? Items(Weapon->MagazineItem) : nullptr;
				Price += Magazine ? Magazine->Price : 0;
			}
			return Price;
		}

		void AddMoney(int32& Money, int32 Amount)
		{
			Money = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Money) + Amount, MAX_int32));
		}
	}

	const TCHAR* Describe(EResult Result)
	{
		switch (Result)
		{
		case EResult::Ok:              return TEXT("Готово");
		case EResult::NotForSale:      return TEXT("Этот торговец такое не продаёт");
		case EResult::NotBought:       return TEXT("Этот торговец такое не покупает");
		case EResult::NotEnoughMoney:  return TEXT("Не хватает билетов");
		case EResult::NoRoom:          return TEXT("В рюкзаке нет свободной ячейки");
		case EResult::Missing:         return TEXT("Такого предмета уже нет");
		case EResult::NothingToRepair: return TEXT("Чинить нечего");
		default:                       return TEXT("");
		}
	}

	const TCHAR* GetTraderName(ERubezhTraderKind Kind)
	{
		switch (Kind)
		{
		case ERubezhTraderKind::Gunsmith:  return TEXT("Оружейник");
		case ERubezhTraderKind::Medic:     return TEXT("Медик");
		case ERubezhTraderKind::Armorer:   return TEXT("Бронник");
		case ERubezhTraderKind::Scavenger: return TEXT("Скупщик");
		default:                           return TEXT("Торговец");
		}
	}

	FString FormatMoney(int32 Amount)
	{
		return FString::Printf(TEXT("%d бил."), Amount);
	}

	bool IsSoldBy(const FRubezhItemDefinition& Item, ERubezhTraderKind Kind)
	{
		return Kind != ERubezhTraderKind::None && Item.Trader == Kind && Item.Price > 0 && Item.bSoldByTrader;
	}

	bool IsBoughtBy(const FRubezhItemDefinition& Item, ERubezhTraderKind Kind)
	{
		return Kind != ERubezhTraderKind::None && Item.Trader == Kind && Item.Price > 0;
	}

	int32 GetSellValue(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Item, FRubezhItemLookup Items, Weapons::FWeaponLookup WeaponStats)
	{
		if (Item.Price <= 0 || Entry.Count <= 0)
		{
			return 0;
		}

		// Износ снижает цену только самого оружия или брони: магазин и патроны внутри оружия не изнашиваются.
		const double Condition = (WeaponStats(Entry.ItemId) || Equipment::CanWear(Item)) ? FMath::Clamp(Entry.Durability, 0.0f, Weapons::MaxDurability) / Weapons::MaxDurability : 1.0;
		const double FullPrice = static_cast<double>(Item.Price) * Entry.Count * Condition + GetLoadedValueBeforeRounding(Entry, Item, Items, WeaponStats);

		// Вниз, чтобы продажа не оказалась выгоднее покупки; но хоть что-то за предмет дают всегда.
		const int64 Value = FMath::FloorToInt64(FullPrice * SellPriceFraction);
		return static_cast<int32>(FMath::Clamp<int64>(Value, 1, MAX_int32));
	}

	int32 GetRepairCost(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Item)
	{
		const float Missing = Weapons::MaxDurability - Entry.Durability;
		if (Missing <= 0.0f || Item.Price <= 0)
		{
			return 0;
		}
		const double Cost = static_cast<double>(Item.Price) * RepairPricePercent * Missing / (100.0 * Weapons::MaxDurability);
		return static_cast<int32>(FMath::Clamp<int64>(FMath::CeilToInt64(Cost), 1, MAX_int32));
	}

	EResult Buy(FRubezhInventoryGrid& Grid, int32& Money, FName ItemId, int32 Count, ERubezhTraderKind Kind, FRubezhItemLookup Items)
	{
		const FRubezhItemDefinition* Item = Items(ItemId);
		if (!Item || Count <= 0 || !IsSoldBy(*Item, Kind))
		{
			return EResult::NotForSale;
		}

		// В 64 битах: запрос клиента не должен переполнить цену и выдать предмет даром.
		const int64 Cost = static_cast<int64>(Item->Price) * Count;
		if (Cost > Money)
		{
			return EResult::NotEnoughMoney;
		}

		FRubezhInventoryGrid Trial = Grid;
		if (Trial.AddItem(ItemId, *Item, Count) > 0)
		{
			return EResult::NoRoom;
		}

		Grid = MoveTemp(Trial);
		Money -= static_cast<int32>(Cost);
		return EResult::Ok;
	}

	EResult Sell(FRubezhInventoryGrid& Grid, int32& Money, int32 EntryIndex, ERubezhTraderKind Kind, FRubezhItemLookup Items, Weapons::FWeaponLookup WeaponStats)
	{
		if (!Grid.GetEntries().IsValidIndex(EntryIndex))
		{
			return EResult::Missing;
		}

		const FRubezhInventoryEntry& Entry = Grid.GetEntries()[EntryIndex];
		const FRubezhItemDefinition* Item = Items(Entry.ItemId);
		if (!Item || !IsBoughtBy(*Item, Kind))
		{
			return EResult::NotBought;
		}

		const int32 Value = GetSellValue(Entry, *Item, Items, WeaponStats);
		FRubezhInventoryEntry Removed;
		if (!Grid.RemoveEntry(EntryIndex, Removed))
		{
			return EResult::Missing;
		}
		AddMoney(Money, Value);
		return EResult::Ok;
	}

	EResult Repair(FRubezhInventoryEntry& Entry, int32& Money, const FRubezhItemDefinition& Item)
	{
		const int32 Cost = GetRepairCost(Entry, Item);
		if (Cost <= 0)
		{
			return EResult::NothingToRepair;
		}
		if (Cost > Money)
		{
			return EResult::NotEnoughMoney;
		}
		Money -= Cost;
		Entry.Durability = Weapons::MaxDurability;
		return EResult::Ok;
	}

	bool CanClean(float Durability)
	{
		return Durability < CleanDurabilityCap;
	}

	float GetCleanedDurability(float Durability, float CleanPercent)
	{
		return CanClean(Durability) ? FMath::Min(Durability + CleanPercent, CleanDurabilityCap) : Durability;
	}
}
