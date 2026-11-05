// РУБЕЖ: Земля после разлома

#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhEquipment.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	using namespace Rubezh::Equipment;

	constexpr EAutomationTestFlags EquipmentTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;
	constexpr int32 Pockets = 4;

	FRubezhItemDefinition MakeGear(ERubezhEquipSlot Slot, float Protection, float Bleed, float WeightKg = 1.0f)
	{
		FRubezhItemDefinition Item;
		Item.Category = ERubezhItemCategory::Equipment;
		Item.EquipSlot = Slot;
		Item.Protection = Protection;
		Item.BleedProtection = Bleed;
		Item.WeightKg = WeightKg;
		return Item;
	}

	FRubezhItemDefinition MakeBackpack(int32 Slots, float CarryBonus)
	{
		FRubezhItemDefinition Item = MakeGear(ERubezhEquipSlot::Backpack, 0.0f, 0.0f);
		Item.BackpackSlots = Slots;
		Item.CarryBonusKg = CarryBonus;
		return Item;
	}

	struct FGearCatalog
	{
		FRubezhItemDefinition Helmet = MakeGear(ERubezhEquipSlot::Head, 40.0f, 50.0f, 1.5f);
		FRubezhItemDefinition Vest = MakeGear(ERubezhEquipSlot::Armor, 50.0f, 60.0f, 8.0f);
		FRubezhItemDefinition Jacket = MakeGear(ERubezhEquipSlot::Top, 10.0f, 20.0f, 1.0f);
		FRubezhItemDefinition Pants = MakeGear(ERubezhEquipSlot::Bottom, 5.0f, 10.0f, 0.5f);
		FRubezhItemDefinition Sack = MakeBackpack(12, 2.0f);
		FRubezhItemDefinition BigPack = MakeBackpack(24, 8.0f);
		FRubezhItemDefinition Brick = MakeGear(ERubezhEquipSlot::None, 0.0f, 0.0f, 3.0f);

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("helmet")) return &Helmet;
			if (Id == TEXT("vest")) return &Vest;
			if (Id == TEXT("jacket")) return &Jacket;
			if (Id == TEXT("pants")) return &Pants;
			if (Id == TEXT("sack")) return &Sack;
			if (Id == TEXT("bigpack")) return &BigPack;
			if (Id == TEXT("brick")) return &Brick;
			return nullptr;
		}
	};

	FEquipped NoEquipment()
	{
		FEquipped Equipped;
		Equipped.SetNum(SlotCount);
		return Equipped;
	}

	int32 FindEntry(const FRubezhInventoryGrid& Grid, FName ItemId)
	{
		return Grid.GetEntries().IndexOfByPredicate([ItemId](const FRubezhInventoryEntry& Entry) { return Entry.ItemId == ItemId; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEquipmentProtectionTest, "Rubezh.Equipment.Protection", EquipmentTestFlags)

bool FRubezhEquipmentProtectionTest::RunTest(const FString& Parameters)
{
	const FGearCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FEquipped Equipped = NoEquipment();
	TestEqual(TEXT("Без снаряжения урон не меняется"), GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup).DamageMultiplier, 1.0f);

	Equipped[GetSlotIndex(ERubezhEquipSlot::Armor)] = TEXT("vest");
	Equipped[GetSlotIndex(ERubezhEquipSlot::Top)] = TEXT("jacket");
	Equipped[GetSlotIndex(ERubezhEquipSlot::Head)] = TEXT("helmet");

	const FProtection Torso = GetProtection(Equipped, ERubezhBodyZone::Torso, Lookup);
	TestEqual(TEXT("Корпус: бронежилет 50 % и куртка 10 % перемножаются → 0,45"), Torso.DamageMultiplier, 0.5f * 0.9f, 0.001f);
	TestEqual(TEXT("Корпус: кровотечение 0,4 × 0,8"), Torso.BleedChanceMultiplier, 0.4f * 0.8f, 0.001f);
	TestEqual(TEXT("Руки закрывает только куртка"), GetProtection(Equipped, ERubezhBodyZone::Arms, Lookup).DamageMultiplier, 0.9f, 0.001f);
	TestEqual(TEXT("Голову — шлем"), GetProtection(Equipped, ERubezhBodyZone::Head, Lookup).DamageMultiplier, 0.6f, 0.001f);
	TestEqual(TEXT("Ноги открыты"), GetProtection(Equipped, ERubezhBodyZone::Legs, Lookup).DamageMultiplier, 1.0f);

	TestTrue(TEXT("Голова — head"), GetZoneForBone(TEXT("head")) == ERubezhBodyZone::Head);
	TestTrue(TEXT("Предплечье — руки"), GetZoneForBone(TEXT("lowerarm_l")) == ERubezhBodyZone::Arms);
	TestTrue(TEXT("Голень — ноги"), GetZoneForBone(TEXT("calf_r")) == ERubezhBodyZone::Legs);
	TestTrue(TEXT("Позвоночник — корпус"), GetZoneForBone(TEXT("spine_03")) == ERubezhBodyZone::Torso);
	TestTrue(TEXT("Без кости — корпус"), GetZoneForBone(NAME_None) == ERubezhBodyZone::Torso);

	TestEqual(TEXT("Вес надетого"), GetWeight(Equipped, Lookup), 10.5f, 0.001f);

	// Защищённое место реже кровоточит.
	FRandomStream Random(42);
	Rubezh::Injuries::FHitInfo Covered{ 15.0f, false, 0.0f };
	int32 Bleeds = 0;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		Bleeds += Rubezh::Injuries::Has(Rubezh::Injuries::RollHitInjuries(Covered, Random), ERubezhInjury::Bleeding) ? 1 : 0;
	}
	TestEqual(TEXT("Полная защита от кровотечения — кровотечений нет"), Bleeds, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEquipmentEquipTest, "Rubezh.Equipment.Equip", EquipmentTestFlags)

bool FRubezhEquipmentEquipTest::RunTest(const FString& Parameters)
{
	const FGearCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	// На персонаже большой рюкзак на 24 ячейки — без него были бы карманы.
	FEquipped Equipped = NoEquipment();
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("bigpack");
	FRubezhInventoryGrid Grid(GetSlotCount(Equipped, Lookup, Pockets));
	Grid.AddItem(TEXT("vest"), Catalog.Vest, 1);		// ячейка 0
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1);	// ячейка 1

	TestTrue(TEXT("Кирпич не надевается"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("brick")), Lookup, Pockets) == EResult::NotWearable);
	TestTrue(TEXT("Бронежилет надет"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("vest")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("…и пропал из рюкзака"), Grid.CountItem(TEXT("vest")), 0);
	TestEqual(TEXT("…и стоит в слоте"), Equipped[GetSlotIndex(ERubezhEquipSlot::Armor)], FName(TEXT("vest")));

	TestTrue(TEXT("Снять в занятую ячейку нельзя"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, 1, Lookup, Pockets) == EResult::NoRoom);
	TestEqual(TEXT("…бронежилет остался надет"), Equipped[GetSlotIndex(ERubezhEquipSlot::Armor)], FName(TEXT("vest")));
	TestTrue(TEXT("Снять в пустую ячейку 7"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, 7, Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("…бронежилет в ячейке 7"), Grid.GetEntries()[FindEntry(Grid, TEXT("vest"))].Slot, 7);
	TestTrue(TEXT("Пустой слот не снять"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, {}, Lookup, Pockets) == EResult::Missing);

	// Смена: надетая куртка возвращается в рюкзак.
	Grid.AddItem(TEXT("jacket"), Catalog.Jacket, 2);
	TestTrue(TEXT("Надели первую куртку"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("jacket")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("В рюкзаке осталась одна"), Grid.CountItem(TEXT("jacket")), 1);
	TestTrue(TEXT("Надели вторую — первая вернулась"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("jacket")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("Курток в рюкзаке по-прежнему одна"), Grid.CountItem(TEXT("jacket")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEquipmentBackpackTest, "Rubezh.Equipment.Backpack", EquipmentTestFlags)

bool FRubezhEquipmentBackpackTest::RunTest(const FString& Parameters)
{
	const FGearCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FEquipped Equipped = NoEquipment();
	TestEqual(TEXT("Без рюкзака — карманы"), GetSlotCount(Equipped, Lookup, Pockets), Pockets);

	// Надет вещмешок на 12 ячеек, в нём большой рюкзак и кирпич.
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("sack");
	FRubezhInventoryGrid Grid(GetSlotCount(Equipped, Lookup, Pockets));
	TestEqual(TEXT("В вещмешке 12 ячеек"), Grid.GetSlotCount(), 12);
	TestEqual(TEXT("Прибавка к весу вещмешка"), GetCarryBonusKg(Equipped, Lookup), 2.0f);
	Grid.AddItem(TEXT("bigpack"), Catalog.BigPack, 1);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1);

	TestTrue(TEXT("Сменили на большой рюкзак"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("bigpack")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("Ячеек стало 24"), Grid.GetSlotCount(), 24);
	TestEqual(TEXT("Вещмешок лёг в рюкзак"), Grid.CountItem(TEXT("sack")), 1);
	TestEqual(TEXT("Кирпич на месте"), Grid.CountItem(TEXT("brick")), 1);
	TestEqual(TEXT("Прибавка большого рюкзака"), GetCarryBonusKg(Equipped, Lookup), 8.0f);
	FName Dropped;

	// 4 вещи займут все 4 кармана — самому рюкзаку места не останется.
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 2);
	TestTrue(TEXT("Рюкзак некуда снять"), Unequip(Grid, Equipped, ERubezhEquipSlot::Backpack, {}, Lookup, Pockets) == EResult::NoRoom);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1);
	TestTrue(TEXT("5 вещей в 4 кармана не влезут"), TakeOff(Grid, Equipped, ERubezhEquipSlot::Backpack, Lookup, Pockets, Dropped) == EResult::WontFit);
	TestEqual(TEXT("…рюкзак не изменился"), Grid.GetSlotCount(), 24);
	TestEqual(TEXT("…и остался надет"), Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)], FName(TEXT("bigpack")));

	// Пустой рюкзак можно снять и выбросить: остаются карманы.
	Grid.RemoveItem(TEXT("brick"), 4);
	Grid.RemoveItem(TEXT("sack"), 1);
	TestTrue(TEXT("Пустой рюкзак снимается"), TakeOff(Grid, Equipped, ERubezhEquipSlot::Backpack, Lookup, Pockets, Dropped) == EResult::Ok);
	TestEqual(TEXT("…сняли именно его"), Dropped, FName(TEXT("bigpack")));
	TestEqual(TEXT("…остались карманы"), Grid.GetSlotCount(), Pockets);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEquipmentDropBagTest, "Rubezh.Equipment.DropBag", EquipmentTestFlags)

bool FRubezhEquipmentDropBagTest::RunTest(const FString& Parameters)
{
	const FGearCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FEquipped Equipped = NoEquipment();
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("sack");
	FRubezhInventoryGrid Grid(12);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1);
	Grid.AddItem(TEXT("jacket"), Catalog.Jacket, 1);

	FName BagId;
	FRubezhInventoryGrid BagContents;
	TestTrue(TEXT("Рюкзак сброшен"), DropBag(Grid, Equipped, Pockets, BagId, BagContents) == EResult::Ok);
	TestEqual(TEXT("…это вещмешок"), BagId, FName(TEXT("sack")));
	TestEqual(TEXT("…вещи ушли с ним"), BagContents.GetEntries().Num(), 2);
	TestTrue(TEXT("…карманы пусты"), Grid.IsEmpty() && Grid.GetSlotCount() == Pockets);
	TestTrue(TEXT("…слот рюкзака пуст"), Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)].IsNone());
	TestTrue(TEXT("Без рюкзака сбрасывать нечего"), DropBag(Grid, Equipped, Pockets, BagId, BagContents) == EResult::Missing);

	// Пока бросил, подобрал штаны в карман. Надеваем рюкзак обратно — штаны перекладываются в него.
	Grid.AddItem(TEXT("pants"), Catalog.Pants, 1);
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("bigpack");
	TestTrue(TEXT("Свой рюкзак надет — чужой не надеть"), WearBag(Grid, Equipped, BagId, BagContents, Lookup) == EResult::SlotTaken);
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = NAME_None;
	TestTrue(TEXT("Кирпич не рюкзак"), WearBag(Grid, Equipped, TEXT("brick"), BagContents, Lookup) == EResult::NotWearable);

	TestTrue(TEXT("Надели обратно"), WearBag(Grid, Equipped, BagId, BagContents, Lookup) == EResult::Ok);
	TestEqual(TEXT("…12 ячеек"), Grid.GetSlotCount(), 12);
	TestEqual(TEXT("…кирпич, куртка и штаны внутри"), Grid.GetEntries().Num(), 3);
	TestEqual(TEXT("…вещмешок надет"), Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)], FName(TEXT("sack")));

	// Полный рюкзак и вещь в кармане — не влезет.
	FRubezhInventoryGrid Full(12);
	Full.AddItem(TEXT("brick"), Catalog.Brick, 12);
	FRubezhInventoryGrid Pocket(Pockets);
	Pocket.AddItem(TEXT("pants"), Catalog.Pants, 1);
	FEquipped Bare = NoEquipment();
	TestTrue(TEXT("Вещи из карманов не влезут в полный рюкзак"), WearBag(Pocket, Bare, TEXT("sack"), Full, Lookup) == EResult::WontFit);
	TestEqual(TEXT("…в кармане остались штаны"), Pocket.CountItem(TEXT("pants")), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
