// РУБЕЖ: Земля после разлома

#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhEquipment.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	using namespace Rubezh::Equipment;

	constexpr EAutomationTestFlags EquipmentTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;
	const FIntPoint Pockets(4, 2);

	FRubezhItemDefinition MakeGear(ERubezhEquipSlot Slot, int32 Width, int32 Height, float Protection, float Bleed, float WeightKg = 1.0f)
	{
		FRubezhItemDefinition Item;
		Item.Category = ERubezhItemCategory::Equipment;
		Item.EquipSlot = Slot;
		Item.Width = Width;
		Item.Height = Height;
		Item.Protection = Protection;
		Item.BleedProtection = Bleed;
		Item.WeightKg = WeightKg;
		return Item;
	}

	FRubezhItemDefinition MakeBackpack(int32 Width, int32 Height, int32 GridWidth, int32 GridHeight, float CarryBonus)
	{
		FRubezhItemDefinition Item = MakeGear(ERubezhEquipSlot::Backpack, Width, Height, 0.0f, 0.0f);
		Item.BackpackWidth = GridWidth;
		Item.BackpackHeight = GridHeight;
		Item.CarryBonusKg = CarryBonus;
		return Item;
	}

	struct FGearCatalog
	{
		FRubezhItemDefinition Helmet = MakeGear(ERubezhEquipSlot::Head, 2, 2, 40.0f, 50.0f, 1.5f);
		FRubezhItemDefinition Vest = MakeGear(ERubezhEquipSlot::Armor, 3, 3, 50.0f, 60.0f, 8.0f);
		FRubezhItemDefinition Jacket = MakeGear(ERubezhEquipSlot::Top, 2, 2, 10.0f, 20.0f, 1.0f);
		FRubezhItemDefinition Pants = MakeGear(ERubezhEquipSlot::Bottom, 2, 2, 5.0f, 10.0f, 0.5f);
		FRubezhItemDefinition Sack = MakeBackpack(2, 2, 5, 4, 2.0f);
		FRubezhItemDefinition BigPack = MakeBackpack(3, 3, 8, 8, 8.0f);
		FRubezhItemDefinition Brick = MakeGear(ERubezhEquipSlot::None, 2, 2, 0.0f, 0.0f, 3.0f);

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

	// На персонаже большой рюкзак 8×8 — без него сетка была бы карманами.
	FEquipped Equipped = NoEquipment();
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("bigpack");
	FRubezhInventoryGrid Grid(GetGridSize(Equipped, Lookup, Pockets));
	Grid.AddItem(TEXT("vest"), Catalog.Vest, 1, Lookup);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1, Lookup);

	TestTrue(TEXT("Кирпич не надевается"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("brick")), Lookup, Pockets) == EResult::NotWearable);
	TestTrue(TEXT("Бронежилет надет"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("vest")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("…и пропал из сетки"), Grid.CountItem(TEXT("vest")), 0);
	TestEqual(TEXT("…и стоит в слоте"), Equipped[GetSlotIndex(ERubezhEquipSlot::Armor)], FName(TEXT("vest")));

	TestTrue(TEXT("Снять на занятое место нельзя"),
		Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, FIntPoint(3, 0), false, Lookup, Pockets) == EResult::NoRoom);
	TestEqual(TEXT("…бронежилет остался надет"), Equipped[GetSlotIndex(ERubezhEquipSlot::Armor)], FName(TEXT("vest")));
	TestTrue(TEXT("Снять куда влезет"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, {}, false, Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("…бронежилет в сетке"), Grid.CountItem(TEXT("vest")), 1);
	TestTrue(TEXT("Пустой слот не снять"), Unequip(Grid, Equipped, ERubezhEquipSlot::Armor, {}, false, Lookup, Pockets) == EResult::Missing);

	// Смена: надетая куртка возвращается в сетку.
	Grid.AddItem(TEXT("jacket"), Catalog.Jacket, 2, Lookup);
	TestTrue(TEXT("Надели первую куртку"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("jacket")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("В сетке осталась одна"), Grid.CountItem(TEXT("jacket")), 1);
	TestTrue(TEXT("Надели вторую — первая вернулась"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("jacket")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("Курток в сетке по-прежнему одна"), Grid.CountItem(TEXT("jacket")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEquipmentBackpackTest, "Rubezh.Equipment.Backpack", EquipmentTestFlags)

bool FRubezhEquipmentBackpackTest::RunTest(const FString& Parameters)
{
	const FGearCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FEquipped Equipped = NoEquipment();
	TestEqual(TEXT("Без рюкзака — карманы"), GetGridSize(Equipped, Lookup, Pockets), Pockets);

	// Вещмешок 5×4 надет, в нём большой рюкзак 3×3 и кирпич.
	Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)] = TEXT("sack");
	FRubezhInventoryGrid Grid(GetGridSize(Equipped, Lookup, Pockets));
	TestEqual(TEXT("Сетка вещмешка 5×4"), Grid.GetSize(), FIntPoint(5, 4));
	TestEqual(TEXT("Прибавка к весу вещмешка"), GetCarryBonusKg(Equipped, Lookup), 2.0f);
	Grid.AddItem(TEXT("bigpack"), Catalog.BigPack, 1, Lookup);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1, Lookup);

	TestTrue(TEXT("Сменили на большой рюкзак"), Equip(Grid, Equipped, FindEntry(Grid, TEXT("bigpack")), Lookup, Pockets) == EResult::Ok);
	TestEqual(TEXT("Сетка стала 8×8"), Grid.GetSize(), FIntPoint(8, 8));
	TestEqual(TEXT("Вещмешок лёг в сетку"), Grid.CountItem(TEXT("sack")), 1);
	TestEqual(TEXT("Кирпич на месте"), Grid.CountItem(TEXT("brick")), 1);
	TestEqual(TEXT("Прибавка большого рюкзака"), GetCarryBonusKg(Equipped, Lookup), 8.0f);
	FName Dropped;

	// Снять рюкзак в карманы 4×2 нельзя: кирпич и вещмешок туда влезут, а сам рюкзак 3×3 — уже нет.
	const FRubezhInventoryGrid Before = Grid;
	TestTrue(TEXT("Рюкзак 3×3 в карманы 4×2 не положить"),
		Unequip(Grid, Equipped, ERubezhEquipSlot::Backpack, {}, false, Lookup, Pockets) == EResult::NoRoom);
	Grid.AddItem(TEXT("brick"), Catalog.Brick, 1, Lookup);
	TestTrue(TEXT("С ещё одним кирпичом вещи не влезут в карманы"),
		TakeOff(Grid, Equipped, ERubezhEquipSlot::Backpack, Lookup, Pockets, Dropped) == EResult::WontFit);
	Grid.RemoveItem(TEXT("brick"), 1);
	TestEqual(TEXT("…сетка не изменилась"), Grid.GetSize(), Before.GetSize());
	TestEqual(TEXT("…рюкзак остался надет"), Equipped[GetSlotIndex(ERubezhEquipSlot::Backpack)], FName(TEXT("bigpack")));

	// Пустой рюкзак можно снять и выбросить: карманы остаются.
	Grid.RemoveItem(TEXT("brick"), 1);
	Grid.RemoveItem(TEXT("sack"), 1);
	TestTrue(TEXT("Пустой рюкзак снимается"), TakeOff(Grid, Equipped, ERubezhEquipSlot::Backpack, Lookup, Pockets, Dropped) == EResult::Ok);
	TestEqual(TEXT("…сняли именно его"), Dropped, FName(TEXT("bigpack")));
	TestEqual(TEXT("…остались карманы"), Grid.GetSize(), Pockets);

	// Resize: вещи на своих местах остаются, вылезающие перекладываются.
	FRubezhInventoryGrid Big(FIntPoint(8, 8));
	Big.AddEntryAt(TEXT("brick"), 1, FIntPoint(0, 0), false, Lookup);
	Big.AddEntryAt(TEXT("jacket"), 1, FIntPoint(6, 6), false, Lookup);
	TestTrue(TEXT("Уменьшение до 4×2 удалось"), Big.Resize(FIntPoint(4, 2), Lookup));
	TestEqual(TEXT("Кирпич остался в (0,0)"), Big.GetEntries()[0].Position, FIntPoint(0, 0));
	TestEqual(TEXT("Куртка переложена в (2,0)"), Big.GetEntries()[1].Position, FIntPoint(2, 0));
	TestFalse(TEXT("В 2×2 обе вещи не влезут"), Big.Resize(FIntPoint(2, 2), Lookup));
	TestEqual(TEXT("…и сетка осталась 4×2"), Big.GetSize(), FIntPoint(4, 2));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
