// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhPickupRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags InventoryTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;

	FRubezhItemDefinition MakeDefinition(int32 Width, int32 Height, int32 MaxStack, float WeightKg, ERubezhItemCategory Category = ERubezhItemCategory::Junk)
	{
		FRubezhItemDefinition Definition;
		Definition.Width = Width;
		Definition.Height = Height;
		Definition.MaxStack = MaxStack;
		Definition.WeightKg = WeightKg;
		Definition.Category = Category;
		return Definition;
	}

	/** Маленький каталог для тестов: автомат 5×2, патроны 1×1 по 60, аккумулятор 2×2. */
	struct FTestCatalog
	{
		FRubezhItemDefinition Rifle = MakeDefinition(5, 2, 1, 3.6f, ERubezhItemCategory::Equipment);
		FRubezhItemDefinition Ammo = MakeDefinition(1, 1, 60, 0.016f, ERubezhItemCategory::Consumable);
		FRubezhItemDefinition Battery = MakeDefinition(2, 2, 1, 13.0f, ERubezhItemCategory::Valuable);

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
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(FIntPoint(6, 8));
	TestEqual(TEXT("Автомат влез целиком"), Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1, Lookup), 0);
	TestEqual(TEXT("Автомат лёг в левый верхний угол"), Grid.GetEntries()[0].Position, FIntPoint(0, 0));
	TestFalse(TEXT("Автомат не повёрнут"), Grid.GetEntries()[0].bRotated);

	TestFalse(TEXT("Клетка под автоматом занята"), Grid.IsAreaFree(FIntPoint(4, 1), FIntPoint(1, 1), Lookup));
	TestTrue(TEXT("Клетка справа от автомата свободна"), Grid.IsAreaFree(FIntPoint(5, 0), FIntPoint(1, 1), Lookup));
	TestFalse(TEXT("Область за краем сетки недоступна"), Grid.IsAreaFree(FIntPoint(5, 7), FIntPoint(2, 1), Lookup));

	// Сетка 2×6: автомат 5×2 влезает только повёрнутым (2×5).
	FRubezhInventoryGrid Narrow(FIntPoint(2, 6));
	TestEqual(TEXT("В узкую сетку автомат влезает"), Narrow.AddItem(TEXT("rifle"), Catalog.Rifle, 1, Lookup), 0);
	TestTrue(TEXT("…повёрнутым"), Narrow.GetEntries()[0].bRotated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryStackingTest, "Rubezh.Inventory.Grid.Stacking", InventoryTestFlags)

bool FRubezhInventoryStackingTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(FIntPoint(2, 1));
	TestEqual(TEXT("40 патронов — одна стопка"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 40, Lookup), 0);
	TestEqual(TEXT("Ещё 50: 20 доложены в стопку, 30 — новая стопка"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 50, Lookup), 0);
	TestEqual(TEXT("Две стопки"), Grid.GetEntries().Num(), 2);
	TestEqual(TEXT("Всего 90 патронов"), Grid.CountItem(TEXT("ammo")), 90);

	TestEqual(TEXT("Сетка 2×1 полна: из 100 влезло только 30"), Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 100, Lookup), 70);
	TestEqual(TEXT("Всего 120 патронов"), Grid.CountItem(TEXT("ammo")), 120);

	TestEqual(TEXT("Убрали 70"), Grid.RemoveItem(TEXT("ammo"), 70), 70);
	TestEqual(TEXT("Осталось 50"), Grid.CountItem(TEXT("ammo")), 50);
	TestEqual(TEXT("Пустая стопка удалена"), Grid.GetEntries().Num(), 1);
	TestEqual(TEXT("Нельзя убрать больше, чем есть"), Grid.RemoveItem(TEXT("ammo"), 500), 50);
	TestEqual(TEXT("Сетка пуста"), Grid.GetEntries().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryMoveTest, "Rubezh.Inventory.Grid.Move", InventoryTestFlags)

bool FRubezhInventoryMoveTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(FIntPoint(6, 8));
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1, Lookup);	// (0,0)–(1,1)
	Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1, Lookup);		// (0,2)–(4,3)

	TestFalse(TEXT("Нельзя положить аккумулятор на автомат"), Grid.MoveEntry(0, FIntPoint(1, 2), false, Lookup));
	TestTrue(TEXT("Можно переложить аккумулятор вниз"), Grid.MoveEntry(0, FIntPoint(0, 6), false, Lookup));
	TestEqual(TEXT("Аккумулятор на новом месте"), Grid.GetEntries()[0].Position, FIntPoint(0, 6));
	TestTrue(TEXT("Можно сдвинуть стопку на частично своё же место"), Grid.MoveEntry(0, FIntPoint(1, 6), false, Lookup));
	TestTrue(TEXT("Автомат можно повернуть, если место есть"), Grid.MoveEntry(1, FIntPoint(4, 0), true, Lookup));
	TestFalse(TEXT("Нельзя вынести стопку за сетку"), Grid.MoveEntry(1, FIntPoint(5, 0), true, Lookup));
	TestFalse(TEXT("Несуществующая стопка"), Grid.MoveEntry(7, FIntPoint(0, 0), false, Lookup));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInventoryRemoveEntryTest, "Rubezh.Inventory.Grid.RemoveEntry", InventoryTestFlags)

bool FRubezhInventoryRemoveEntryTest::RunTest(const FString& Parameters)
{
	const FTestCatalog Catalog;
	auto Lookup = [&Catalog](FName Id) { return Catalog.Find(Id); };

	FRubezhInventoryGrid Grid(FIntPoint(6, 8));
	Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 90, Lookup);		// стопки 60 и 30
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1, Lookup);

	FRubezhInventoryEntry Removed;
	TestTrue(TEXT("Вынули вторую стопку патронов"), Grid.RemoveEntry(1, Removed));
	TestEqual(TEXT("…это патроны"), Removed.ItemId, FName(TEXT("ammo")));
	TestEqual(TEXT("…все 30 штук"), Removed.Count, 30);
	TestEqual(TEXT("Патронов осталось 60"), Grid.CountItem(TEXT("ammo")), 60);
	TestEqual(TEXT("Аккумулятор на месте"), Grid.CountItem(TEXT("battery")), 1);
	TestFalse(TEXT("Несуществующая стопка"), Grid.RemoveEntry(5, Removed));
	TestTrue(TEXT("Место освободилось — туда снова можно положить"), Grid.IsAreaFree(FIntPoint(1, 0), FIntPoint(1, 1), Lookup));
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

	FRubezhInventoryGrid Grid(FIntPoint(6, 8));
	TestEqual(TEXT("Пустой рюкзак ничего не весит"), Grid.GetTotalWeight(Lookup), 0.0f);

	Grid.AddItem(TEXT("rifle"), Catalog.Rifle, 1, Lookup);
	Grid.AddItem(TEXT("ammo"), Catalog.Ammo, 120, Lookup);
	Grid.AddItem(TEXT("battery"), Catalog.Battery, 1, Lookup);
	// 3.6 + 120 × 0.016 + 13 = 18.52 кг
	TestEqual(TEXT("Вес автомата, 120 патронов и аккумулятора"), Grid.GetTotalWeight(Lookup), 18.52f, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
