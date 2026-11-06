// РУБЕЖ: Земля после разлома
// Износ брони: защита падает вместе с прочностью, прочность ходит за предметом, ремонт у бронника.

#include "Items/RubezhEquipment.h"
#include "Items/RubezhTradeRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags GearWearTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	struct FWearTestCatalog
	{
		TMap<FName, FRubezhItemDefinition> Items;

		FWearTestCatalog()
		{
			auto Add = [this](const TCHAR* Id, ERubezhEquipSlot Slot, float Protection, float Bleed, int32 Price)
			{
				FRubezhItemDefinition Item;
				Item.EquipSlot = Slot;
				Item.Protection = Protection;
				Item.BleedProtection = Bleed;
				Item.Price = Price;
				Item.Trader = ERubezhTraderKind::Armorer;
				Item.MaxStack = 1;
				Item.WeightKg = 1.0f;
				Items.Add(FName(Id), Item);
			};
			Add(TEXT("vest"), ERubezhEquipSlot::Armor, 50.0f, 40.0f, 1000);
			Add(TEXT("vest2"), ERubezhEquipSlot::Armor, 20.0f, 0.0f, 100);
			Add(TEXT("jacket"), ERubezhEquipSlot::Top, 10.0f, 0.0f, 100);
			Add(TEXT("helmet"), ERubezhEquipSlot::Head, 40.0f, 0.0f, 100);
			Add(TEXT("bag"), ERubezhEquipSlot::Backpack, 0.0f, 0.0f, 100);
		}

		const FRubezhItemDefinition* Find(FName Id) const { return Items.Find(Id); }
	};

	Rubezh::Equipment::FEquipped MakeWearTestEquipped(std::initializer_list<TPair<ERubezhEquipSlot, FName>> Gear)
	{
		Rubezh::Equipment::FEquipped Equipped;
		Equipped.SetNum(Rubezh::Equipment::SlotCount);
		for (const TPair<ERubezhEquipSlot, FName>& Piece : Gear)
		{
			Equipped[Rubezh::Equipment::GetSlotIndex(Piece.Key)] = Piece.Value;
		}
		return Equipped;
	}

	Rubezh::Equipment::FEquippedWear MakeFullWear()
	{
		Rubezh::Equipment::FEquippedWear Wear;
		Wear.Init(Rubezh::Equipment::MaxGearDurability, Rubezh::Equipment::SlotCount);
		return Wear;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhGearWearProtectionTest, "Rubezh.GearWear.ProtectionFallsWithDurability", GearWearTestFlags)
bool FRubezhGearWearProtectionTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Equipment;
	const FWearTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };
	const FEquipped Equipped = MakeWearTestEquipped({ { ERubezhEquipSlot::Armor, FName("vest") } });
	FEquippedWear Wear = MakeFullWear();
	const int32 Vest = GetSlotIndex(ERubezhEquipSlot::Armor);

	TestEqual(TEXT("Целый жилет: 50 %"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup, &Wear).DamageMultiplier, 0.5f, 0.001f);
	TestEqual(TEXT("Без прочности считается целым"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup).DamageMultiplier, 0.5f, 0.001f);

	Wear[Vest] = 50.0f;
	TestEqual(TEXT("На половине прочности — половина защиты"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup, &Wear).DamageMultiplier, 0.75f, 0.001f);
	TestEqual(TEXT("Защита от кровотечения тоже падает"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup, &Wear).BleedChanceMultiplier, 0.8f, 0.001f);

	Wear[Vest] = 0.0f;
	TestEqual(TEXT("Разбитый жилет не защищает"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup, &Wear).DamageMultiplier, 1.0f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhGearWearHitTest, "Rubezh.GearWear.HitWearsCoveringLayers", GearWearTestFlags)
bool FRubezhGearWearHitTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Equipment;
	const FWearTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };
	const FEquipped Equipped = MakeWearTestEquipped({ { ERubezhEquipSlot::Armor, FName("vest") }, { ERubezhEquipSlot::Top, FName("jacket") },
		{ ERubezhEquipSlot::Head, FName("helmet") }, { ERubezhEquipSlot::Backpack, FName("bag") } });
	FEquippedWear Wear = MakeFullWear();

	ApplyHitWear(Equipped, Wear, ERubezhBodyZone::Torso, 50.0f, Lookup);
	TestEqual(TEXT("Жилет износился"), Wear[GetSlotIndex(ERubezhEquipSlot::Armor)], 90.0f, 0.001f);
	TestEqual(TEXT("Куртка на корпусе тоже"), Wear[GetSlotIndex(ERubezhEquipSlot::Top)], 90.0f, 0.001f);
	TestEqual(TEXT("Шлем цел: попали не в голову"), Wear[GetSlotIndex(ERubezhEquipSlot::Head)], 100.0f);
	TestEqual(TEXT("Рюкзак не изнашивается"), Wear[GetSlotIndex(ERubezhEquipSlot::Backpack)], 100.0f);

	ApplyHitWear(Equipped, Wear, ERubezhBodyZone::Torso, 100000.0f, Lookup);
	TestEqual(TEXT("Ниже нуля не падает"), Wear[GetSlotIndex(ERubezhEquipSlot::Armor)], 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhGearWearTravelsTest, "Rubezh.GearWear.DurabilityTravelsWithItem", GearWearTestFlags)
bool FRubezhGearWearTravelsTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Equipment;
	const FWearTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };
	const int32 VestSlot = GetSlotIndex(ERubezhEquipSlot::Armor);

	FRubezhInventoryGrid Grid(6);
	FEquipped Equipped = MakeWearTestEquipped({ { ERubezhEquipSlot::Armor, FName("vest") } });
	FEquippedWear Wear = MakeFullWear();
	Wear[VestSlot] = 40.0f;

	TestTrue(TEXT("Снял"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, {}, Lookup, 4, &Wear) == EResult::Ok);
	TestEqual(TEXT("В рюкзаке та же прочность"), Grid.GetEntries()[0].Durability, 40.0f);

	TestTrue(TEXT("Надел обратно"), Equip(Grid, Equipped, 0, Lookup, 4, &Wear) == EResult::Ok);
	TestEqual(TEXT("Надетый с той же прочностью"), Wear[VestSlot], 40.0f);

	// Замена: старый уходит в рюкзак со своей прочностью, новый приходит со своей.
	FRubezhInventoryEntry Other;
	Other.ItemId = FName("vest2");
	Other.Count = 1;
	Other.Durability = 70.0f;
	Grid.AddEntry(Other, *Catalog.Find(Other.ItemId));
	TestTrue(TEXT("Заменил"), Equip(Grid, Equipped, 0, Lookup, 4, &Wear) == EResult::Ok);
	TestEqual(TEXT("Новый — 70 %"), Wear[VestSlot], 70.0f);
	TestEqual(TEXT("Старый в рюкзаке — 40 %"), Grid.GetEntries()[0].Durability, 40.0f);

	TestTrue(TEXT("Снял в ячейку 3"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, TOptional<int32>(3), Lookup, 4, &Wear) == EResult::Ok);
	TestEqual(TEXT("В ячейке 3 — 70 %"), Grid.GetEntries()[Grid.FindEntryAt(3)].Durability, 70.0f);
	TestEqual(TEXT("Пустой слот считается целым"), Wear[VestSlot], 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhGearWearTradeTest, "Rubezh.GearWear.SellAndRepair", GearWearTestFlags)
bool FRubezhGearWearTradeTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Trade;
	const FWearTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };
	auto NoWeapons = [](FName) -> const FRubezhWeaponDefinition* { return nullptr; };
	const FRubezhItemDefinition& Vest = *Catalog.Find(FName("vest"));

	FRubezhInventoryEntry Entry;
	Entry.ItemId = FName("vest");
	Entry.Count = 1;
	Entry.Durability = 40.0f;

	TestEqual(TEXT("Изношенный жилет дешевле: 1000 × 40 % × 50 %"), GetSellValue(Entry, Vest, Lookup, NoWeapons), 200);
	TestEqual(TEXT("Ремонт: 1000 × 30 % × 60 %"), GetRepairCost(Entry, Vest), 180);

	int32 Money = 100;
	TestTrue(TEXT("Денег мало"), Repair(Entry, Money, Vest) == Rubezh::Trade::EResult::NotEnoughMoney);
	TestEqual(TEXT("Отказ не чинит"), Entry.Durability, 40.0f);

	Money = 200;
	TestTrue(TEXT("Починил"), Repair(Entry, Money, Vest) == Rubezh::Trade::EResult::Ok);
	TestEqual(TEXT("Деньги списаны"), Money, 20);
	TestEqual(TEXT("Прочность полная"), Entry.Durability, 100.0f);

	FRubezhInventoryEntry Bag;
	Bag.ItemId = FName("bag");
	Bag.Count = 1;
	Bag.Durability = 10.0f;
	TestEqual(TEXT("Рюкзак от прочности не дешевеет"), GetSellValue(Bag, *Catalog.Find(Bag.ItemId), Lookup, NoWeapons), 50);
	return true;
}

#endif
