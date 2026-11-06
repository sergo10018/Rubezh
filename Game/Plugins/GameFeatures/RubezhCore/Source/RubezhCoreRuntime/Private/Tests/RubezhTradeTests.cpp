// РУБЕЖ: Земля после разлома

#include "Items/RubezhTradeRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags TradeTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;

	FRubezhItemDefinition MakeTradeItem(int32 Price, ERubezhTraderKind Trader, bool bSold, int32 MaxStack = 1)
	{
		FRubezhItemDefinition Item;
		Item.Price = Price;
		Item.Trader = Trader;
		Item.bSoldByTrader = bSold;
		Item.MaxStack = MaxStack;
		return Item;
	}

	struct FTradeCatalog
	{
		FRubezhItemDefinition Rifle = MakeTradeItem(1000, ERubezhTraderKind::Gunsmith, true);
		FRubezhItemDefinition Sniper = MakeTradeItem(3000, ERubezhTraderKind::Gunsmith, false);
		FRubezhItemDefinition Ammo = MakeTradeItem(4, ERubezhTraderKind::Gunsmith, true, 60);
		FRubezhItemDefinition Magazine = MakeTradeItem(80, ERubezhTraderKind::Gunsmith, true);
		FRubezhItemDefinition Bandage = MakeTradeItem(15, ERubezhTraderKind::Medic, true, 5);
		FRubezhItemDefinition Bolts = MakeTradeItem(2, ERubezhTraderKind::Scavenger, false, 20);
		FRubezhItemDefinition Junk = MakeTradeItem(0, ERubezhTraderKind::Scavenger, false);
		FRubezhWeaponDefinition RifleStats;

		FTradeCatalog()
		{
			Rifle.AmmoItem = TEXT("ammo");
			Magazine.AmmoItem = TEXT("ammo");
			Magazine.MagazineCapacity = 30;
			RifleStats.AmmoItem = TEXT("ammo");
			RifleStats.MagazineItem = TEXT("mag");
		}

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("rifle")) return &Rifle;
			if (Id == TEXT("sniper")) return &Sniper;
			if (Id == TEXT("ammo")) return &Ammo;
			if (Id == TEXT("mag")) return &Magazine;
			if (Id == TEXT("bandage")) return &Bandage;
			if (Id == TEXT("bolts")) return &Bolts;
			if (Id == TEXT("junk")) return &Junk;
			return nullptr;
		}

		const FRubezhWeaponDefinition* FindWeapon(FName Id) const
		{
			return Id == TEXT("rifle") ? &RifleStats : nullptr;
		}
	};

	FRubezhInventoryEntry MakeTradeEntry(FName Id, int32 Count = 1, int32 Rounds = 0, float Durability = 100.0f, bool bHasMagazine = false)
	{
		FRubezhInventoryEntry Entry;
		Entry.ItemId = Id;
		Entry.Count = Count;
		Entry.Rounds = Rounds;
		Entry.Durability = Durability;
		Entry.bHasMagazine = bHasMagazine;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhTradeBuyTest, "Rubezh.Trade.Buy", TradeTestFlags)

bool FRubezhTradeBuyTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Trade;
	const FTradeCatalog Catalog;
	auto Items = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(2);
	int32 Money = 100;

	TestTrue(TEXT("Покупка за свои деньги"), Buy(Grid, Money, TEXT("ammo"), 10, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Деньги списаны"), Money, 60);
	TestEqual(TEXT("Патроны в рюкзаке"), Grid.CountItem(TEXT("ammo")), 10);

	TestTrue(TEXT("Не хватает денег"), Buy(Grid, Money, TEXT("ammo"), 16, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotEnoughMoney);
	TestEqual(TEXT("После отказа деньги на месте"), Money, 60);
	TestEqual(TEXT("После отказа патронов столько же"), Grid.CountItem(TEXT("ammo")), 10);

	TestTrue(TEXT("Этот торговец такое не продаёт"), Buy(Grid, Money, TEXT("bandage"), 1, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotForSale);
	TestTrue(TEXT("Редкое оружие не продаётся"), Buy(Grid, Money, TEXT("sniper"), 1, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotForSale);
	TestTrue(TEXT("Неизвестный предмет не продаётся"), Buy(Grid, Money, TEXT("nothing"), 1, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotForSale);
	TestTrue(TEXT("Ноль штук не покупается"), Buy(Grid, Money, TEXT("ammo"), 0, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotForSale);
	TestTrue(TEXT("Отрицательное число штук не даёт денег"), Buy(Grid, Money, TEXT("ammo"), -5, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NotForSale);
	TestEqual(TEXT("Отрицательная покупка не изменила деньги"), Money, 60);

	// Огромное число штук не должно переполнить цену и дать предмет даром.
	int32 Rich = 100;
	TestTrue(TEXT("Огромная покупка отклонена"), Buy(Grid, Rich, TEXT("ammo"), MAX_int32, ERubezhTraderKind::Gunsmith, Items) != Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Огромная покупка не изменила деньги"), Rich, 100);

	// Купленное оружие — новое и незаряженное.
	FRubezhInventoryGrid WeaponGrid(1);
	int32 Enough = 2000;
	TestTrue(TEXT("Автомат куплен"), Buy(WeaponGrid, Enough, TEXT("rifle"), 1, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Автомат с полной прочностью"), WeaponGrid.GetEntries()[0].Durability, 100.0f);
	TestEqual(TEXT("Автомат без патронов"), WeaponGrid.GetEntries()[0].Rounds, 0);

	TestTrue(TEXT("Нет свободной ячейки"), Buy(WeaponGrid, Enough, TEXT("mag"), 1, ERubezhTraderKind::Gunsmith, Items) == Rubezh::Trade::EResult::NoRoom);
	TestEqual(TEXT("Без места деньги не списаны"), Enough, 1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhTradeSellTest, "Rubezh.Trade.Sell", TradeTestFlags)

bool FRubezhTradeSellTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Trade;
	const FTradeCatalog Catalog;
	auto Items = [&Catalog](FName Id) { return Catalog.Find(Id); };
	auto Weapons = [&Catalog](FName Id) { return Catalog.FindWeapon(Id); };

	FRubezhInventoryGrid Grid(6);
	Grid.PlaceEntry(MakeTradeEntry(TEXT("bolts"), 10), 0);
	Grid.PlaceEntry(MakeTradeEntry(TEXT("bandage"), 2), 1);
	Grid.PlaceEntry(MakeTradeEntry(TEXT("junk")), 2);
	int32 Money = 0;

	TestEqual(TEXT("Стопка стоит половину цены"), GetSellValue(Grid.GetEntries()[0], Catalog.Bolts, Items, Weapons), 10);

	TestTrue(TEXT("Медик не покупает хлам"), Sell(Grid, Money, Grid.FindEntryAt(0), ERubezhTraderKind::Medic, Items, Weapons) == Rubezh::Trade::EResult::NotBought);
	TestEqual(TEXT("После отказа денег нет"), Money, 0);
	TestEqual(TEXT("После отказа стопка на месте"), Grid.CountItem(TEXT("bolts")), 10);

	TestTrue(TEXT("Скупщик покупает хлам"), Sell(Grid, Money, Grid.FindEntryAt(0), ERubezhTraderKind::Scavenger, Items, Weapons) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Деньги начислены"), Money, 10);
	TestEqual(TEXT("Стопка продана целиком"), Grid.CountItem(TEXT("bolts")), 0);

	TestTrue(TEXT("Предмет без цены не покупают"), Sell(Grid, Money, Grid.FindEntryAt(2), ERubezhTraderKind::Scavenger, Items, Weapons) == Rubezh::Trade::EResult::NotBought);
	TestTrue(TEXT("Нет такой стопки"), Sell(Grid, Money, 99, ERubezhTraderKind::Scavenger, Items, Weapons) == Rubezh::Trade::EResult::Missing);

	// Износ снижает цену оружия; магазин и патроны внутри оплачиваются отдельно.
	TestEqual(TEXT("Изношенное оружие дешевле"), GetSellValue(MakeTradeEntry(TEXT("rifle"), 1, 0, 50.0f), Catalog.Rifle, Items, Weapons), 250);
	TestEqual(TEXT("Магазин с патронами"), GetSellValue(MakeTradeEntry(TEXT("mag"), 1, 30), Catalog.Magazine, Items, Weapons), 40 + 60);
	TestEqual(TEXT("Оружие с магазином и патронами"), GetSellValue(MakeTradeEntry(TEXT("rifle"), 1, 10, 100.0f, true), Catalog.Rifle, Items, Weapons), 500 + 40 + 20);

	// Даже полностью изношенное оружие можно сдать хоть за что-то.
	TestEqual(TEXT("Убитое оружие — минимальная цена"), GetSellValue(MakeTradeEntry(TEXT("rifle"), 1, 0, 0.0f), Catalog.Rifle, Items, Weapons), 1);

	// Счёт не переполняется.
	FRubezhInventoryGrid RichGrid(1);
	RichGrid.PlaceEntry(MakeTradeEntry(TEXT("mag")), 0);
	int32 Rich = MAX_int32 - 1;
	TestTrue(TEXT("Продажа при почти полном счёте"), Sell(RichGrid, Rich, 0, ERubezhTraderKind::Gunsmith, Items, Weapons) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Счёт упирается в максимум"), Rich, MAX_int32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhTradeNoProfitTest, "Rubezh.Trade.NoProfitLoop", TradeTestFlags)

bool FRubezhTradeNoProfitTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Trade;
	const FTradeCatalog Catalog;
	auto Items = [&Catalog](FName Id) { return Catalog.Find(Id); };
	auto Weapons = [&Catalog](FName Id) { return Catalog.FindWeapon(Id); };

	// Купить и сразу продать никогда не выгодно: на этом держится экономика.
	for (const FName Id : { FName(TEXT("rifle")), FName(TEXT("ammo")), FName(TEXT("mag")), FName(TEXT("bandage")) })
	{
		FRubezhInventoryGrid Grid(1);
		const int32 Start = 10000;
		int32 Money = Start;
		TestTrue(TEXT("Куплено"), Buy(Grid, Money, Id, 1, Catalog.Find(Id)->Trader, Items) == Rubezh::Trade::EResult::Ok);
		TestTrue(TEXT("Продано"), Sell(Grid, Money, 0, Catalog.Find(Id)->Trader, Items, Weapons) == Rubezh::Trade::EResult::Ok);
		TestTrue(FString::Printf(TEXT("%s: нет прибыли"), *Id.ToString()), Money < Start);
	}

	// Сложение цены магазина с патронами тоже не должно обгонять цену покупки по отдельности.
	const FRubezhInventoryEntry Loaded = MakeTradeEntry(TEXT("mag"), 1, 30);
	TestTrue(TEXT("Полный магазин дешевле своих частей"), GetSellValue(Loaded, Catalog.Magazine, Items, Weapons) < Catalog.Magazine.Price + 30 * Catalog.Ammo.Price);

	// Отремонтировать и продать не дороже, чем стоит новое оружие.
	FRubezhInventoryEntry Worn = MakeTradeEntry(TEXT("rifle"), 1, 0, 10.0f);
	int32 Money = 10000;
	TestTrue(TEXT("Ремонт"), Repair(Worn, Money, Catalog.Rifle) == Rubezh::Trade::EResult::Ok);
	TestTrue(TEXT("Отремонтированное стоит меньше нового"), GetSellValue(Worn, Catalog.Rifle, Items, Weapons) < Catalog.Rifle.Price);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhTradeRepairTest, "Rubezh.Trade.RepairAndClean", TradeTestFlags)

bool FRubezhTradeRepairTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Trade;
	const FTradeCatalog Catalog;

	TestEqual(TEXT("Ремонт половины прочности"), GetRepairCost(MakeTradeEntry(TEXT("rifle"), 1, 0, 50.0f), Catalog.Rifle), 150);
	TestEqual(TEXT("Целое оружие чинить не за что"), GetRepairCost(MakeTradeEntry(TEXT("rifle")), Catalog.Rifle), 0);
	TestEqual(TEXT("Даже малый износ чего-то стоит"), GetRepairCost(MakeTradeEntry(TEXT("rifle"), 1, 0, 99.99f), Catalog.Rifle), 1);

	FRubezhInventoryEntry Weapon = MakeTradeEntry(TEXT("rifle"), 1, 7, 50.0f, true);
	int32 Money = 100;
	TestTrue(TEXT("Не хватает на ремонт"), Repair(Weapon, Money, Catalog.Rifle) == Rubezh::Trade::EResult::NotEnoughMoney);
	TestEqual(TEXT("Отказ не меняет прочность"), Weapon.Durability, 50.0f);
	TestEqual(TEXT("Отказ не тратит деньги"), Money, 100);

	Money = 200;
	TestTrue(TEXT("Ремонт сделан"), Repair(Weapon, Money, Catalog.Rifle) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Прочность полная"), Weapon.Durability, 100.0f);
	TestEqual(TEXT("Деньги списаны"), Money, 50);
	TestEqual(TEXT("Патроны не тронуты"), Weapon.Rounds, 7);
	TestTrue(TEXT("Повторно чинить нечего"), Repair(Weapon, Money, Catalog.Rifle) == Rubezh::Trade::EResult::NothingToRepair);

	// Чистка: прибавка, но не выше потолка; выше потолка не помогает.
	TestEqual(TEXT("Чистка прибавляет"), GetCleanedDurability(30.0f, 15.0f), 45.0f);
	TestEqual(TEXT("Чистка не выше потолка"), GetCleanedDurability(65.0f, 15.0f), CleanDurabilityCap);
	TestEqual(TEXT("Чистка не снижает прочность"), GetCleanedDurability(90.0f, 15.0f), 90.0f);
	TestTrue(TEXT("Изношенное можно почистить"), CanClean(30.0f));
	TestFalse(TEXT("На потолке чистить незачем"), CanClean(CleanDurabilityCap));
	return true;
}

#endif
