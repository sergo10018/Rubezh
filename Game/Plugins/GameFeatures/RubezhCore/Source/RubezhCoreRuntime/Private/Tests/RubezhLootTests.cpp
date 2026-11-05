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

	struct FLootCatalog
	{
		FRubezhItemDefinition Rifle = MakeItem(1, ERubezhItemCategory::Equipment);
		FRubezhItemDefinition Ammo = MakeItem(60, ERubezhItemCategory::Consumable);
		FRubezhItemDefinition Scrap = MakeItem(1, ERubezhItemCategory::Junk);
		FRubezhItemDefinition Documents = MakeItem(1, ERubezhItemCategory::Valuable);

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("rifle")) return &Rifle;
			if (Id == TEXT("ammo")) return &Ammo;
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

	FRubezhInventoryGrid Backpack(12);
	Backpack.AddEntryAt(TEXT("rifle"), 1, 0, Lookup);
	Backpack.AddEntryAt(TEXT("ammo"), 45, 2, Lookup);
	Backpack.AddEntryAt(TEXT("scrap"), 1, 5, Lookup);
	Backpack.AddEntryAt(TEXT("documents"), 1, 7, Lookup);

	const FRubezhInventoryGrid Body = TakeLostOnDeath(Backpack, Lookup);
	TestEqual(TEXT("На теле патроны, хлам и документы"), Body.GetEntries().Num(), 3);
	TestEqual(TEXT("…ровно три ячейки"), Body.GetSlotCount(), 3);
	TestEqual(TEXT("…патронов 45"), Body.CountItem(TEXT("ammo")), 45);
	TestEqual(TEXT("…разложены с первой ячейки"), Body.GetEntries()[0].Slot, 0);

	TestEqual(TEXT("У игрока остался автомат (экипировка)"), Backpack.CountItem(TEXT("rifle")), 1);
	TestEqual(TEXT("…на своём месте"), Backpack.GetEntries()[0].Slot, 0);
	TestEqual(TEXT("…и больше ничего"), Backpack.GetEntries().Num(), 1);
	TestEqual(TEXT("Рюкзак не уменьшился"), Backpack.GetSlotCount(), 12);

	FRubezhInventoryGrid OnlyGear(4);
	OnlyGear.AddEntryAt(TEXT("rifle"), 1, 0, Lookup);
	TestTrue(TEXT("Только экипировка — тела нет"), TakeLostOnDeath(OnlyGear, Lookup).IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
