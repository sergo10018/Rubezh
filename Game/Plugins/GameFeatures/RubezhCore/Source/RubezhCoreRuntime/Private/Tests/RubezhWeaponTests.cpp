// РУБЕЖ: Земля после разлома

#include "Items/RubezhLootRules.h"
#include "Items/RubezhWeaponRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags WeaponTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;

	FRubezhItemDefinition MakeItem(int32 MaxStack, float Weight)
	{
		FRubezhItemDefinition Item;
		Item.MaxStack = MaxStack;
		Item.WeightKg = Weight;
		return Item;
	}

	struct FCatalog
	{
		FRubezhItemDefinition Rifle = MakeItem(1, 3.0f);
		FRubezhItemDefinition Pistol = MakeItem(1, 1.0f);
		FRubezhItemDefinition Ammo = MakeItem(60, 0.01f);
		FRubezhItemDefinition Magazine = MakeItem(1, 0.3f);
		FRubezhWeaponDefinition RifleStats;
		FRubezhWeaponDefinition PistolStats;
		FRubezhWeaponDefinition ShotgunStats;

		FCatalog()
		{
			Magazine.AmmoItem = TEXT("ammo");
			Magazine.MagazineCapacity = 30;
			RifleStats.AmmoItem = TEXT("ammo");
			RifleStats.MagazineItem = TEXT("mag");
			PistolStats.WeaponClass = ERubezhWeaponClass::Sidearm;
			ShotgunStats.AmmoItem = TEXT("ammo");
			ShotgunStats.TubeCapacity = 6;
		}

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("rifle")) return &Rifle;
			if (Id == TEXT("pistol")) return &Pistol;
			if (Id == TEXT("ammo")) return &Ammo;
			if (Id == TEXT("mag")) return &Magazine;
			return nullptr;
		}

		const FRubezhWeaponDefinition* FindWeapon(FName Id) const
		{
			if (Id == TEXT("rifle")) return &RifleStats;
			if (Id == TEXT("pistol")) return &PistolStats;
			return nullptr;
		}
	};

	FRubezhInventoryEntry MakeEntry(FName Id, int32 Rounds = 0, float Durability = 100.0f, bool bHasMagazine = false)
	{
		FRubezhInventoryEntry Entry;
		Entry.ItemId = Id;
		Entry.Count = 1;
		Entry.Rounds = Rounds;
		Entry.Durability = Durability;
		Entry.bHasMagazine = bHasMagazine;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhWeaponSlotsTest, "Rubezh.Weapons.Slots", WeaponTestFlags)

bool FRubezhWeaponSlotsTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;
	const FCatalog Catalog;
	auto Weapons = [&Catalog](FName Id) { return Catalog.FindWeapon(Id); };

	FRubezhInventoryGrid Grid(4);
	Grid.PlaceEntry(MakeEntry(TEXT("rifle"), 12, 40.0f, true), 2);
	Grid.PlaceEntry(MakeEntry(TEXT("pistol"), 8, 90.0f, true), 3);
	Grid.PlaceEntry(MakeEntry(TEXT("mag")), 0);
	FWeaponSlots Slots;

	TestTrue(TEXT("Магазин — не оружие"), Equip(Grid, Slots, Grid.FindEntryAt(0), {}, Weapons) == Rubezh::Weapons::EResult::NotWeapon);
	TestTrue(TEXT("Автомат не берётся в слот пистолета"), Equip(Grid, Slots, Grid.FindEntryAt(2), SidearmSlot, Weapons) == Rubezh::Weapons::EResult::WrongSlot);
	TestEqual(TEXT("После отказа оружие в рюкзаке"), Grid.GetEntries().Num(), 3);
	TestTrue(TEXT("Пистолет не берётся в основной слот"), Equip(Grid, Slots, Grid.FindEntryAt(3), 0, Weapons) == Rubezh::Weapons::EResult::WrongSlot);

	TestTrue(TEXT("Автомат взят"), Equip(Grid, Slots, Grid.FindEntryAt(2), {}, Weapons) == Rubezh::Weapons::EResult::Ok);
	TestEqual(TEXT("Автомат занял первый основной слот"), Slots[0].ItemId, FName(TEXT("rifle")));
	TestEqual(TEXT("Патроны сохранились"), Slots[0].Rounds, 12);
	TestEqual(TEXT("Прочность сохранилась"), Slots[0].Durability, 40.0f);
	TestTrue(TEXT("Пистолет взят"), Equip(Grid, Slots, Grid.FindEntryAt(3), {}, Weapons) == Rubezh::Weapons::EResult::Ok);
	TestEqual(TEXT("Пистолет в своём слоте"), Slots[SidearmSlot].ItemId, FName(TEXT("pistol")));

	// Снять в занятую ячейку: ложится в свободную, состояние при этом не теряется.
	TestTrue(TEXT("Автомат снят"), Unequip(Grid, Slots, 0, 0) == Rubezh::Weapons::EResult::Ok);
	TestTrue(TEXT("Слот свободен"), Slots[0].ItemId.IsNone());
	const FRubezhInventoryEntry& Back = Grid.GetEntries()[Grid.FindEntryAt(1)];
	TestEqual(TEXT("Снятый автомат в первой свободной ячейке"), Back.ItemId, FName(TEXT("rifle")));
	TestEqual(TEXT("У снятого те же патроны"), Back.Rounds, 12);

	// Второй автомат в занятый слот: меняются местами.
	Grid.PlaceEntry(MakeEntry(TEXT("rifle"), 3, 70.0f, true), 2);
	TestTrue(TEXT("Автомат взят снова"), Equip(Grid, Slots, Grid.FindEntryAt(1), 1, Weapons) == Rubezh::Weapons::EResult::Ok);
	TestTrue(TEXT("Замена"), Equip(Grid, Slots, Grid.FindEntryAt(2), 1, Weapons) == Rubezh::Weapons::EResult::Ok);
	TestEqual(TEXT("В слоте новый"), Slots[1].Rounds, 3);
	TestEqual(TEXT("Старый вернулся в ячейку нового"), Grid.GetEntries()[Grid.FindEntryAt(2)].Rounds, 12);

	FRubezhInventoryGrid Full(1);
	Full.PlaceEntry(MakeEntry(TEXT("mag")), 0);
	TestTrue(TEXT("Некуда снять"), Unequip(Full, Slots, 1, {}) == Rubezh::Weapons::EResult::NoRoom);
	TestEqual(TEXT("После отказа оружие в слоте"), Slots[1].ItemId, FName(TEXT("rifle")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhWeaponReloadTest, "Rubezh.Weapons.Reload", WeaponTestFlags)

bool FRubezhWeaponReloadTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;
	const FCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(4);
	Grid.PlaceEntry(MakeEntry(TEXT("mag"), 10), 0);
	Grid.PlaceEntry(MakeEntry(TEXT("mag"), 25), 1);
	Grid.PlaceEntry(MakeEntry(TEXT("mag"), 0), 2);
	TestEqual(TEXT("Запас считает патроны в магазинах"), CountSpareRounds(Grid, Catalog.RifleStats), 35);

	// Без магазина: самый полный уходит в оружие и освобождает ячейку.
	FRubezhInventoryEntry Rifle = MakeEntry(TEXT("rifle"));
	TestTrue(TEXT("Зарядили"), Reload(Grid, Rifle, Catalog.RifleStats, Lookup));
	TestEqual(TEXT("Самый полный"), Rifle.Rounds, 25);
	TestTrue(TEXT("Магазин в оружии"), Rifle.bHasMagazine);
	TestEqual(TEXT("Ячейка освободилась"), Grid.FindEntryAt(1), INDEX_NONE);

	// С магазином: снятый ложится туда, откуда взят новый, со своими патронами.
	Rifle.Rounds = 3;
	TestTrue(TEXT("Сменили магазин"), Reload(Grid, Rifle, Catalog.RifleStats, Lookup));
	TestEqual(TEXT("В оружии 10"), Rifle.Rounds, 10);
	TestEqual(TEXT("Снятый с 3 патронами"), Grid.GetEntries()[Grid.FindEntryAt(0)].Rounds, 3);

	// Пустые магазины не ставятся.
	FRubezhInventoryGrid Empty(1);
	Empty.PlaceEntry(MakeEntry(TEXT("mag"), 0), 0);
	FRubezhInventoryEntry Other = MakeEntry(TEXT("rifle"), 5, 100.0f, true);
	TestFalse(TEXT("Нечем"), Reload(Empty, Other, Catalog.RifleStats, Lookup));
	TestEqual(TEXT("Патроны на месте"), Other.Rounds, 5);

	// Дробовик заряжается патронами из рюкзака, только до вместимости.
	FRubezhInventoryGrid Shells(2);
	Shells.AddItem(TEXT("ammo"), Catalog.Ammo, 10);
	FRubezhInventoryEntry Shotgun = MakeEntry(TEXT("rifle"), 2);
	TestTrue(TEXT("Дозарядили"), Reload(Shells, Shotgun, Catalog.ShotgunStats, Lookup));
	TestEqual(TEXT("До вместимости"), Shotgun.Rounds, 6);
	TestEqual(TEXT("Остаток в рюкзаке"), Shells.CountItem(TEXT("ammo")), 6);
	TestEqual(TEXT("Запас дробовика — патроны россыпью"), CountSpareRounds(Shells, Catalog.ShotgunStats), 6);
	TestFalse(TEXT("Полный не заряжается"), Reload(Shells, Shotgun, Catalog.ShotgunStats, Lookup));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhMagazineLoadingTest, "Rubezh.Weapons.MagazineLoading", WeaponTestFlags)

bool FRubezhMagazineLoadingTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;
	const FCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(3);
	Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 2);
	Grid.PlaceEntry(MakeEntry(TEXT("mag"), 28), 1);
	const int32 MagazineCell = 1;

	TestTrue(TEXT("Первый патрон"), LoadRound(Grid, Grid.FindEntryAt(MagazineCell), Lookup));
	// Последний патрон убирает стопку и сдвигает индексы — магазин должен получить его всё равно.
	TestTrue(TEXT("Второй патрон"), LoadRound(Grid, Grid.FindEntryAt(MagazineCell), Lookup));
	TestEqual(TEXT("Магазин полон"), Grid.GetEntries()[Grid.FindEntryAt(MagazineCell)].Rounds, 30);
	TestEqual(TEXT("Патроны кончились"), Grid.CountItem(TEXT("ammo")), 0);
	TestFalse(TEXT("Полный не набивается"), LoadRound(Grid, Grid.FindEntryAt(MagazineCell), Lookup));

	TestEqual(TEXT("Разрядили"), UnloadMagazine(Grid, Grid.FindEntryAt(MagazineCell), Lookup), 30);
	TestEqual(TEXT("Патроны вернулись"), Grid.CountItem(TEXT("ammo")), 30);
	TestEqual(TEXT("Магазин пуст"), Grid.GetEntries()[Grid.FindEntryAt(MagazineCell)].Rounds, 0);

	// Вес магазина с патронами: сам магазин + патроны в нём.
	FRubezhInventoryGrid Weighted(1);
	Weighted.PlaceEntry(MakeEntry(TEXT("mag"), 30), 0);
	TestEqual(TEXT("Вес с патронами"), Weighted.GetTotalWeight(Lookup), 0.3f + 30 * 0.01f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhItemStateTransferTest, "Rubezh.Weapons.StateSurvivesTransfer", WeaponTestFlags)

bool FRubezhItemStateTransferTest::RunTest(const FString& Parameters)
{
	const FCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Crate(2);
	Crate.PlaceEntry(MakeEntry(TEXT("rifle"), 7, 33.0f, true), 0);
	Crate.PlaceEntry(MakeEntry(TEXT("mag"), 12), 1);
	FRubezhInventoryGrid Backpack(4);

	TestEqual(TEXT("В пустую ячейку"), Rubezh::Loot::Transfer(Crate, 0, Backpack, 2, Lookup), 1);
	TestEqual(TEXT("Куда влезет"), Rubezh::Loot::Transfer(Crate, 0, Backpack, {}, Lookup), 1);
	const FRubezhInventoryEntry& Rifle = Backpack.GetEntries()[Backpack.FindEntryAt(2)];
	TestEqual(TEXT("Прочность цела"), Rifle.Durability, 33.0f);
	TestEqual(TEXT("Патроны целы"), Rifle.Rounds, 7);
	TestTrue(TEXT("Магазин цел"), Rifle.bHasMagazine);
	TestEqual(TEXT("Патроны магазина целы"), Backpack.GetEntries()[Backpack.FindEntryAt(0)].Rounds, 12);

	// Магазины со своими патронами не складываются в стопку.
	FRubezhInventoryGrid Two(3);
	Two.PlaceEntry(MakeEntry(TEXT("mag"), 5), 0);
	Two.PlaceEntry(MakeEntry(TEXT("mag"), 9), 1);
	TestTrue(TEXT("Перенос на такой же магазин"), Two.MoveEntry(0, 1, Lookup));
	TestEqual(TEXT("Поменялись местами, а не слились"), Two.GetEntries().Num(), 2);
	TestEqual(TEXT("Патроны не потерялись"), Two.GetEntries()[Two.FindEntryAt(1)].Rounds, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhWeaponWearTest, "Rubezh.Weapons.Wear", WeaponTestFlags)

bool FRubezhWeaponWearTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;

	TestEqual(TEXT("Новое без осечек"), GetMisfireChance(100.0f), 0.0f);
	TestEqual(TEXT("На 50 % без осечек"), GetMisfireChance(50.0f), 0.0f);
	TestEqual(TEXT("На 35 % половина от 3 %"), GetMisfireChance(35.0f), 0.015f, 0.0001f);
	TestEqual(TEXT("На 20 % — 3 %"), GetMisfireChance(20.0f), 0.03f, 0.0001f);
	TestEqual(TEXT("На нуле — 10 %"), GetMisfireChance(0.0f), 0.10f, 0.0001f);
	TestEqual(TEXT("Выстрел изнашивает"), ApplyWear(100.0f), 99.95f, 0.001f);
	TestEqual(TEXT("Ниже нуля нельзя"), ApplyWear(0.01f), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhWeaponDamageTest, "Rubezh.Weapons.Damage", WeaponTestFlags)

bool FRubezhWeaponDamageTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;

	TestEqual(TEXT("Голова"), GetZoneDamageMultiplier(ERubezhBodyZone::Head), 2.5f);
	TestEqual(TEXT("Торс"), GetZoneDamageMultiplier(ERubezhBodyZone::Torso), 1.0f);
	TestEqual(TEXT("Рука"), GetZoneDamageMultiplier(ERubezhBodyZone::Arms), 0.7f);
	TestEqual(TEXT("Нога"), GetZoneDamageMultiplier(ERubezhBodyZone::Legs), 0.7f);

	TestEqual(TEXT("В пределах дальности — полный"), GetDistanceMultiplier(2000.0f, 25.0f), 1.0f);
	TestEqual(TEXT("На полторы дальности"), GetDistanceMultiplier(3750.0f, 25.0f), 0.75f, 0.001f);
	TestEqual(TEXT("На двух дальностях — половина"), GetDistanceMultiplier(5000.0f, 25.0f), 0.5f, 0.001f);
	TestEqual(TEXT("Дальше не падает"), GetDistanceMultiplier(100000.0f, 25.0f), 0.5f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhWeaponRecoilTest, "Rubezh.Weapons.Recoil", WeaponTestFlags)

bool FRubezhWeaponRecoilTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Weapons;

	FRubezhWeaponDefinition Weapon;
	Weapon.RecoilPitch = 1.0f;
	Weapon.RecoilYaw = 0.5f;
	Weapon.RecoilGrowth = 0.1f;

	TestEqual(TEXT("Первый выстрел"), ComputeRecoil(Weapon, 0, false, 0.0f).Pitch, 1.0f);
	TestEqual(TEXT("Очередь растёт"), ComputeRecoil(Weapon, 5, false, 0.0f).Pitch, 1.5f, 0.001f);
	TestEqual(TEXT("Рост ограничен"), ComputeRecoil(Weapon, 100, false, 0.0f).Pitch, 2.0f, 0.001f);
	TestEqual(TEXT("Присед уменьшает"), ComputeRecoil(Weapon, 0, true, 0.0f).Pitch, 0.7f, 0.001f);
	TestEqual(TEXT("Увод вбок по знаку случайного числа"), ComputeRecoil(Weapon, 0, false, -1.0f).Yaw, -0.5f, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
