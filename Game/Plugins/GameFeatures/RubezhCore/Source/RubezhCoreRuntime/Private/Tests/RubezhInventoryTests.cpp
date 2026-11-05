// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhPickupRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags InventoryTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;

	FRubezhItemDefinition MakeDefinition(int32 MaxStack, float WeightKg, ERubezhItemCategory Category = ERubezhItemCategory::Junk)
	{
		FRubezhItemDefinition Definition;
		Definition.MaxStack = MaxStack;
		Definition.WeightKg = WeightKg;
		Definition.Category = Category;
		return Definition;
	}

	/** Маленький каталог для тестов: автомат, патроны по 60 в стопке, аккумулятор. */
	struct FTestCatalog
	{
		FRubezhItemDefinition Rifle = MakeDefinition(1, 3.6f, ERubezhItemCategory::Equipment);
		FRubezhItemDefinition Ammo = MakeDefinition(60, 0.016f, ERubezhItemCategory::Consumable);
		FRubezhItemDefinition Battery = MakeDefinition(1, 13.0f, ERubezhItemCategory::Valuable);

		const FRubezhItemDefinition* Find(FName Id) const
		{
			if (Id == TEXT("rifle")) return &Rifle;
			if (Id == TEXT("ammo")) return &Ammo;
			if (Id == TEXT("battery")) return &Battery;
			return nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryPlacementTest, "Rubezh.Inventory.Grid.Placement", InventoryTestFlags)

bool FRubezhInventoryPlacementTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;

	FRubezhInventoryGrid Grid(4);
	TestEqual(TEXT("Автомат влез"), Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1), 0);
	TestEqual(TEXT("…в первую ячейку"), Grid.GetEntries()[0].Slot, 0);
	TestEqual(TEXT("Ещё три автомата влезли — каждый в свою ячейку"), Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 3), 0);
	TestEqual(TEXT("Заняты все 4 ячейки"), Grid.GetEntries().Num(), 4);
	TestEqual(TEXT("Пятый не влез"), Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1), 1);
	TestEqual(TEXT("Свободных ячеек нет"), Grid.FindFreeSlot(), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("Ячейка за пределами рюкзака недоступна"), Grid.IsSlotFree(4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryStackingTest, "Rubezh.Inventory.Grid.Stacking", InventoryTestFlags)

bool FRubezhInventoryStackingTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;

	FRubezhInventoryGrid Grid(2);
	TestEqual(TEXT("40 патронов — одна стопка"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 40), 0);
	TestEqual(TEXT("Ещё 50: 20 доложены в стопку, 30 — новая стопка"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 50), 0);
	TestEqual(TEXT("Две стопки"), Grid.GetEntries().Num(), 2);
	TestEqual(TEXT("Всего 90 патронов"), Grid.CountItem(TEXT("ammo")), 90);

	TestEqual(TEXT("2 ячейки полны: из 100 влезло только 30"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 100), 70);
	TestEqual(TEXT("Всего 120 патронов"), Grid.CountItem(TEXT("ammo")), 120);

	TestEqual(TEXT("Убрали 70"), Grid.RemoveItem(TEXT("ammo"), 70), 70);
	TestEqual(TEXT("Осталось 50"), Grid.CountItem(TEXT("ammo")), 50);
	TestEqual(TEXT("Пустая стопка удалена"), Grid.GetEntries().Num(), 1);
	TestEqual(TEXT("Нельзя убрать больше, чем есть"), Grid.RemoveItem(TEXT("ammo"), 500), 50);
	TestTrue(TEXT("Рюкзак пуст"), Grid.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryMoveTest, "Rubezh.Inventory.Grid.Move", InventoryTestFlags)

bool FRubezhInventoryMoveTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(6);
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1);	// ячейка 0
	Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1);		// ячейка 1

	TestTrue(TEXT("Аккумулятор — в пустую ячейку 5"), Grid.MoveEntry(0, 5, Lookup));
	TestEqual(TEXT("…он там"), Grid.GetEntries()[0].Slot, 5);
	TestTrue(TEXT("Автомат на аккумулятор — меняются местами"), Grid.MoveEntry(1, 5, Lookup));
	TestEqual(TEXT("…автомат в 5"), Grid.GetEntries()[1].Slot, 5);
	TestEqual(TEXT("…аккумулятор в 1"), Grid.GetEntries()[0].Slot, 1);
	TestFalse(TEXT("За пределы рюкзака нельзя"), Grid.MoveEntry(0, 6, Lookup));
	TestFalse(TEXT("Несуществующая стопка"), Grid.MoveEntry(7, 0, Lookup));

	// Патроны на патроны — докладываются в стопку.
	FRubezhInventoryGrid Ammo(3);
	Ammo.AddEntryAt(TEXT("ammo"), 50, 0, Lookup);
	Ammo.AddEntryAt(TEXT("ammo"), 30, 2, Lookup);
	TestTrue(TEXT("30 патронов на стопку из 50"), Ammo.MoveEntry(1, 0, Lookup));
	TestEqual(TEXT("…в стопке 60 (больше нельзя)"), Ammo.GetEntries()[0].Count, 60);
	TestEqual(TEXT("…20 остались на месте"), Ammo.GetEntries()[1].Count, 20);
	TestEqual(TEXT("…в ячейке 2"), Ammo.GetEntries()[1].Slot, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryRemoveEntryTest, "Rubezh.Inventory.Grid.RemoveEntry", InventoryTestFlags)

bool FRubezhInventoryRemoveEntryTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;

	FRubezhInventoryGrid Grid(6);
	Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 90);		// стопки 60 и 30
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1);

	FRubezhInventoryEntry Removed;
	TestTrue(TEXT("Вынули вторую стопку патронов"), Grid.RemoveEntry(1, Removed));
	TestEqual(TEXT("…это патроны"), Removed.ItemId, FName(TEXT("ammo")));
	TestEqual(TEXT("…все 30 штук"), Removed.Count, 30);
	TestEqual(TEXT("Патронов осталось 60"), Grid.CountItem(TEXT("ammo")), 60);
	TestEqual(TEXT("Аккумулятор на месте"), Grid.CountItem(TEXT("battery")), 1);
	TestFalse(TEXT("Несуществующая стопка"), Grid.RemoveEntry(5, Removed));
	TestTrue(TEXT("Ячейка освободилась"), Grid.IsSlotFree(1));
	TestEqual(TEXT("…и её займёт следующий предмет"), Grid.FindFreeSlot(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryResizeTest, "Rubezh.Inventory.Grid.Resize", InventoryTestFlags)

bool FRubezhInventoryResizeTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(12);
	Grid.AddEntryAt(TEXT("battery"), 1, 0, Lookup);
	Grid.AddEntryAt(TEXT("rifle"), 1, 10, Lookup);
	TestTrue(TEXT("Уменьшение до 4 ячеек удалось"), Grid.Resize(4));
	TestEqual(TEXT("Аккумулятор остался в 0"), Grid.GetEntries()[0].Slot, 0);
	TestEqual(TEXT("Автомат переехал в 1"), Grid.GetEntries()[1].Slot, 1);
	TestFalse(TEXT("В 1 ячейку два предмета не влезут"), Grid.Resize(1));
	TestEqual(TEXT("…и рюкзак остался на 4"), Grid.GetSlotCount(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhPickupRulesTest, "Rubezh.Inventory.Pickup.Rules", InventoryTestFlags)

bool FRubezhPickupRulesTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Pickup;

	const FVector Pawn(0.0f, 0.0f, 90.0f);
	const FVector Camera(-300.0f, 0.0f, 150.0f);	// камера от третьего лица — за спиной
	const FVector Forward(1.0f, 0.0f, -0.15f);

	TestTrue(TEXT("Предмет у ног — дотянуться можно"), IsWithinReach(Pawn, FVector(100.0f, 0.0f, 0.0f), false));
	TestFalse(TEXT("Предмет в 3 м — нельзя"), IsWithinReach(Pawn, FVector(300.0f, 0.0f, 0.0f), false));
	TestTrue(TEXT("Сервер прощает небольшое расхождение"), IsWithinReach(Pawn, FVector(240.0f, 0.0f, 90.0f), true));
	TestFalse(TEXT("…но не подбор через полкарты"), IsWithinReach(Pawn, FVector(1000.0f, 0.0f, 0.0f), true));

	TestTrue(TEXT("Предмет перед персонажем — цель"), ScoreCandidate(Camera, Forward, Pawn, FVector(120.0f, 0.0f, 0.0f)).IsSet());
	TestFalse(TEXT("Предмет за спиной — не цель"), ScoreCandidate(Camera, Forward, Pawn, FVector(-150.0f, 0.0f, 0.0f)).IsSet());
	TestFalse(TEXT("Предмет далеко впереди — не цель"), ScoreCandidate(Camera, Forward, Pawn, FVector(600.0f, 0.0f, 0.0f)).IsSet());

	const TOptional<float> Straight = ScoreCandidate(Camera, Forward, Pawn, FVector(120.0f, 0.0f, 0.0f));
	const TOptional<float> Aside = ScoreCandidate(Camera, Forward, Pawn, FVector(100.0f, 90.0f, 0.0f));
	TestTrue(TEXT("Предмет прямо по взгляду лучше, чем сбоку"), Straight.IsSet() && Aside.IsSet() && Straight.GetValue() < Aside.GetValue());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryWeightTest, "Rubezh.Inventory.Grid.Weight", InventoryTestFlags)

bool FRubezhInventoryWeightTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(12);
	TestEqual(TEXT("Пустой рюкзак ничего не весит"), Grid.GetTotalWeight(Lookup), 0.0f);

	Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1);
	Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 120);
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1);
	// 3.6 + 120 × 0.016 + 13 = 18.52 кг
	TestEqual(TEXT("Вес автомата, 120 патронов и аккумулятора"), Grid.GetTotalWeight(Lookup), 18.52f, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
