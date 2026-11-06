// РУБЕЖ: Земля после разлома

#include "Items/RubezhLootRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags LootTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;

	FRubezhItemDefinition MakeItem(int32 MaxStack, ERubezhItemCategory Category)
	{
		FRubezhItemDefinition Item;
		Item.MaxStack = MaxStack;
		Item.Category = Category;
		return Item;
	}

	FRubezhItemDefinition MakeMagazine()
	{
		FRubezhItemDefinition Item = MakeItem(1, ERubezhItemCategory::Equipment);
		Item.AmmoItem = TEXT("ammo");
		Item.MagazineCapacity = 30;
		return Item;
	}

	FRubezhInventoryEntry MakeLoadedMagazine(int32 Rounds, int32 Slot)
	{
		FRubezhInventoryEntry Entry;
		Entry.ItemId = TEXT("mag");
		Entry.Count = 1;
		Entry.Rounds = Rounds;
		Entry.Slot = Slot;
		return Entry;
	}

	struct FLootCatalog
	{
		FRubezhItemDefinition Rifle = MakeItem(1, ERubezhItemCategory::Equipment);
		FRubezhItemDefinition Ammo = MakeItem(60, ERubezhItemCategory::Ammo);
		FRubezhItemDefinition Magazine = MakeMagazine();
		FRubezhItemDefinition Bandage = MakeItem(5, ERubezhItemCategory::Consumable);
		FRubezhItemDefinition Scrap = MakeItem(1, ERubezhItemCategory::Junk);
		FRubezhItemDefinition Documents = MakeItem(1, ERubezhItemCategory::Valuable);

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("rifle")) return &Rifle;
			if (Id == TEXT("ammo")) return &Ammo;
			if (Id == TEXT("mag")) return &Magazine;
			if (Id == TEXT("bandage")) return &Bandage;
			if (Id == TEXT("scrap")) return &Scrap;
			if (Id == TEXT("documents")) return &Documents;
			return nullptr;
		}
	};

	FRubezhLootTableRow MakeRow(FName Table, FName ItemId, float Chance, int32 MinCount, int32 MaxCount)
	{
		FRubezhLootTableRow Row;
		Row.Table = Table;
		Row.ItemId = ItemId;
		Row.Chance = Chance;
		Row.MinCount = MinCount;
		Row.MaxCount = MaxCount;
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootRollTest, "Rubezh.Loot.Roll", LootTestFlags)

bool FRubezhLootRollTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Loot;

	const TArray<FRubezhLootTableRow> Rows = {
		MakeRow(TEXT("military"), TEXT("ammo"), 100.0f, 20, 20),
		MakeRow(TEXT("military"), TEXT("rifle"), 0.0f, 1, 1),
		MakeRow(TEXT("medical"), TEXT("scrap"), 100.0f, 1, 1),
		MakeRow(TEXT("military"), TEXT("documents"), 50.0f, 1, 3),
	};

	FRandomStream Random(7);
	const TArray<FDrop> Drops = Roll(Rows, TEXT("military"), Random);
	TestTrue(TEXT("Шанс 100 % — выпадает всегда"), Drops.ContainsByPredicate([](const FDrop& Drop) { return Drop.ItemId == TEXT("ammo") && Drop.Count == 20; }));
	TestFalse(TEXT("Шанс 0 % — никогда"), Drops.ContainsByPredicate([](const FDrop& Drop) { return Drop.ItemId == TEXT("rifle"); }));
	TestFalse(TEXT("Строки другого типа места не учитываются"), Drops.ContainsByPredicate([](const FDrop& Drop) { return Drop.ItemId == TEXT("scrap"); }));

	int32 DocumentDrops = 0;
	bool bCountInRange = true;
	for (int32 Attempt = 0; Attempt < 1000; ++Attempt)
	{
		for (const FDrop& Drop : Roll(Rows, TEXT("military"), Random))
		{
			if (Drop.ItemId == TEXT("documents"))
			{
				++DocumentDrops;
				bCountInRange &= Drop.Count >= 1 && Drop.Count <= 3;
			}
		}
	}
	TestTrue(TEXT("Шанс 50 % — примерно в половине ящиков"), DocumentDrops > 420 && DocumentDrops < 580);
	TestTrue(TEXT("Количество в пределах MinCount..MaxCount"), bCountInRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootTransferTest, "Rubezh.Loot.Transfer", LootTestFlags)

bool FRubezhLootTransferTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Loot;
	const FLootCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Crate(4);
	Crate.AddEntryAt(TEXT("ammo"), 50, 0, Lookup);
	Crate.AddEntryAt(TEXT("rifle"), 1, 3, Lookup);

	FRubezhInventoryGrid Backpack(2);
	Backpack.AddEntryAt(TEXT("ammo"), 40, 1, Lookup);

	TestEqual(TEXT("Патроны на свою стопку: доложено 20 (больше 60 нельзя)"), Transfer(Crate, 0, Backpack, 1, Lookup), 20);
	TestEqual(TEXT("…в стопке рюкзака 60"), Backpack.GetEntries()[0].Count, 60);
	TestEqual(TEXT("…в ящике осталось 30"), Crate.CountItem(TEXT("ammo")), 30);

	TestEqual(TEXT("Остаток — «куда влезет»: в пустую ячейку 0"), Transfer(Crate, 0, Backpack, {}, Lookup), 30);
	TestEqual(TEXT("…в ящике патронов нет"), Crate.CountItem(TEXT("ammo")), 0);
	TestEqual(TEXT("…в рюкзаке 90"), Backpack.CountItem(TEXT("ammo")), 90);

	const int32 RifleIndex = Crate.FindEntryAt(3);
	TestEqual(TEXT("Рюкзак полон — автомат не забрать"), Transfer(Crate, RifleIndex, Backpack, {}, Lookup), 0);
	TestEqual(TEXT("…автомат остался в ящике"), Crate.CountItem(TEXT("rifle")), 1);
	TestEqual(TEXT("Несуществующая стопка"), Transfer(Crate, 9, Backpack, {}, Lookup), 0);

	// Обратно в ящик, в занятую другим предметом ячейку — ляжет в свободную.
	TestEqual(TEXT("Патроны в ячейку с автоматом — легли рядом"), Transfer(Backpack, 0, Crate, 3, Lookup), 60);
	TestEqual(TEXT("…автомат на месте"), Crate.GetEntries()[Crate.FindEntryAt(3)].ItemId, FName(TEXT("rifle")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootDeathTest, "Rubezh.Loot.LostOnDeath", LootTestFlags)

bool FRubezhLootDeathTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Loot;
	const FLootCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	constexpr int32 Deaths = 2000;
	int32 BandageDrops = 0;
	int32 ScrapDrops = 0;
	int32 DocumentDrops = 0;
	int32 BothDrops = 0;
	FRandomStream Random(1);
	for (int32 Death = 0; Death < Deaths; ++Death)
	{
		FRubezhInventoryGrid Backpack(12);
		Backpack.AddEntryAt(TEXT("rifle"), 1, 0, Lookup);
		Backpack.AddEntryAt(TEXT("bandage"), 3, 2, Lookup);
		Backpack.AddEntryAt(TEXT("scrap"), 1, 5, Lookup);
		Backpack.AddEntryAt(TEXT("documents"), 1, 7, Lookup);

		const FRubezhInventoryGrid Body = TakeLostOnDeath(Backpack, Lookup, Random);
		const int32 BandagesOnBody = Body.CountItem(TEXT("bandage"));
		TestTrue(TEXT("Стопка бинтов уходит только целиком"), BandagesOnBody == 0 || BandagesOnBody == 3);
		TestEqual(TEXT("Бинты не размножились и не пропали"), BandagesOnBody + Backpack.CountItem(TEXT("bandage")), 3);
		TestEqual(TEXT("Автомат (экипировка) всегда остаётся"), Backpack.CountItem(TEXT("rifle")), 1);
		TestEqual(TEXT("…на своём месте"), Backpack.GetEntries()[0].Slot, 0);
		TestEqual(TEXT("На теле ровно столько ячеек, сколько стопок"), Body.GetSlotCount(), Body.GetEntries().Num());
		TestEqual(TEXT("Рюкзак не уменьшился"), Backpack.GetSlotCount(), 12);

		const bool bBandage = BandagesOnBody > 0;
		const bool bScrap = Body.CountItem(TEXT("scrap")) > 0;
		BandageDrops += bBandage ? 1 : 0;
		ScrapDrops += bScrap ? 1 : 0;
		DocumentDrops += Body.CountItem(TEXT("documents"));
		BothDrops += bBandage && bScrap ? 1 : 0;
	}

	const auto IsAboutThirtyPercent = [](int32 Count) { return Count > Deaths * 27 / 100 && Count < Deaths * 33 / 100; };
	TestTrue(TEXT("Медицина выпадает примерно в 30 %"), IsAboutThirtyPercent(BandageDrops));
	TestTrue(TEXT("Хлам выпадает примерно в 30 %"), IsAboutThirtyPercent(ScrapDrops));
	TestTrue(TEXT("Ценности выпадают примерно в 30 %"), IsAboutThirtyPercent(DocumentDrops));
	TestTrue(TEXT("Каждая стопка бросает свой шанс: обе — примерно в 9 %"), BothDrops > Deaths * 6 / 100 && BothDrops < Deaths * 12 / 100);

	FRubezhInventoryGrid OnlyGear(4);
	OnlyGear.AddEntryAt(TEXT("rifle"), 1, 0, Lookup);
	TestTrue(TEXT("Только экипировка — тела нет"), TakeLostOnDeath(OnlyGear, Lookup, Random).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootAmmoAmountTest, "Rubezh.Loot.AmmoLossAmount", LootTestFlags)

bool FRubezhLootAmmoAmountTest::RunTest(const FString& Parameters)
{
	using Rubezh::Loot::ComputeAmmoLoss;
	TestEqual(TEXT("100 патронов, 5 %"), ComputeAmmoLoss(100, 5.0f), 5);
	TestEqual(TEXT("100 патронов, 7 %"), ComputeAmmoLoss(100, 7.0f), 7);
	TestEqual(TEXT("Округление: 55 патронов, 5 % — 3"), ComputeAmmoLoss(55, 5.0f), 3);
	TestEqual(TEXT("Мало патронов — может не выпасть ни одного"), ComputeAmmoLoss(8, 5.0f), 0);
	TestEqual(TEXT("…а может один"), ComputeAmmoLoss(8, 7.0f), 1);
	TestEqual(TEXT("Нет патронов — ничего"), ComputeAmmoLoss(0, 7.0f), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootAmmoTakeTest, "Rubezh.Loot.AmmoLossTake", LootTestFlags)

bool FRubezhLootAmmoTakeTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Loot;
	const FLootCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	// 2 россыпью + 5 + 8 в магазинах = 15; 40 % = 6: 2 россыпью, потом 4 из наименее полного магазина.
	FRubezhInventoryGrid Backpack(6);
	Backpack.AddEntryAt(TEXT("ammo"), 2, 0, Lookup);
	Backpack.PlaceEntry(MakeLoadedMagazine(8, 1), 1);
	Backpack.PlaceEntry(MakeLoadedMagazine(5, 2), 2);

	const TArray<FDrop> Lost = TakeAmmoLoss(Backpack, Lookup, 40.0f);
	TestEqual(TEXT("Выпал один калибр"), Lost.Num(), 1);
	TestEqual(TEXT("…6 патронов"), Lost[0].Count, 6);
	TestEqual(TEXT("Россыпь кончилась"), Backpack.CountItem(TEXT("ammo")), 0);
	TestEqual(TEXT("Магазины остались"), Backpack.CountItem(TEXT("mag")), 2);
	TestEqual(TEXT("Полный магазин не тронут"), Backpack.GetEntries()[Backpack.FindEntryAt(1)].Rounds, 8);
	TestEqual(TEXT("Из неполного вынуто 4"), Backpack.GetEntries()[Backpack.FindEntryAt(2)].Rounds, 1);

	FRubezhInventoryGrid Empty(2);
	Empty.PlaceEntry(MakeLoadedMagazine(0, 0), 0);
	TestEqual(TEXT("Пустой магазин ничего не теряет"), TakeAmmoLoss(Empty, Lookup, 7.0f).Num(), 0);

	FRubezhInventoryGrid Few(2);
	Few.AddEntryAt(TEXT("ammo"), 3, 0, Lookup);
	TestEqual(TEXT("Из 3 патронов при 5 % не выпадает ничего"), TakeAmmoLoss(Few, Lookup, 5.0f).Num(), 0);
	TestEqual(TEXT("…и они на месте"), Few.CountItem(TEXT("ammo")), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhLootAmmoChanceTest, "Rubezh.Loot.AmmoLossChance", LootTestFlags)

bool FRubezhLootAmmoChanceTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Loot;
	const FLootCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	constexpr int32 Deaths = 2000;
	constexpr int32 Rounds = 1000;	// 50–70 патронов: выпавшее бывает больше стопки в 60
	int32 Dropped = 0;
	FRandomStream Random(12345);
	for (int32 Death = 0; Death < Deaths; ++Death)
	{
		FRubezhInventoryGrid Backpack(30);
		Backpack.AddItem(TEXT("ammo"), Catalog.Ammo, Rounds);
		Backpack.PlaceEntry(MakeLoadedMagazine(30, 29), 29);

		const FRubezhInventoryGrid Body = TakeLostOnDeath(Backpack, Lookup, Random);
		TestEqual(TEXT("Магазин всегда остаётся"), Backpack.CountItem(TEXT("mag")), 1);
		if (Body.IsEmpty())
		{
			TestEqual(TEXT("Не выпало — все патроны на месте"), Backpack.CountItem(TEXT("ammo")), Rounds);
			continue;
		}

		++Dropped;
		const int32 Lost = Body.CountItem(TEXT("ammo"));
		TestTrue(TEXT("Выпало 5–7 % от всех патронов (с магазином)"), Lost >= 51 && Lost <= 72);
		TestEqual(TEXT("Патроны не размножились"), Lost + Backpack.CountItem(TEXT("ammo")), Rounds);
		for (const FRubezhInventoryEntry& Stack : Body.GetEntries())
		{
			TestTrue(TEXT("Стопка на теле не больше предела"), Stack.Count <= Catalog.Ammo.MaxStack);
		}
	}
	TestTrue(TEXT("Выпадает примерно в 30 % смертей"), Dropped > Deaths * 27 / 100 && Dropped < Deaths * 33 / 100);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
