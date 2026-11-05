// РУБЕЖ: Земля после разлома

#include "Combat/RubezhCombatLogoutSubsystem.h"
#include "Items/RubezhItemCategory.h"
#include "Misc/AutomationTest.h"
#include "Survival/RubezhEncumbrance.h"
#include "Survival/RubezhStaminaRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags RubezhTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEncumbranceLevelTest, "Rubezh.Survival.Encumbrance.Level", RubezhTestFlags)

bool FRubezhEncumbranceLevelTest::RunTest(const FString& Parameters)
{
	using Rubezh::Encumbrance::GetLevel;
	constexpr float Max = Rubezh::Encumbrance::BaseMaxCarryWeight;

	TestTrue(TEXT("Пустой персонаж"), GetLevel(0.0f, Max) == ERubezhEncumbranceLevel::Normal);
	TestTrue(TEXT("Ровно 60 % — ещё норма"), GetLevel(18.0f, Max) == ERubezhEncumbranceLevel::Normal);
	TestTrue(TEXT("Чуть выше 60 %"), GetLevel(18.1f, Max) == ERubezhEncumbranceLevel::Burdened);
	TestTrue(TEXT("Ровно 100 %"), GetLevel(30.0f, Max) == ERubezhEncumbranceLevel::Burdened);
	TestTrue(TEXT("Перегруз"), GetLevel(35.0f, Max) == ERubezhEncumbranceLevel::Overloaded);
	TestTrue(TEXT("Ровно 130 %"), GetLevel(39.0f, Max) == ERubezhEncumbranceLevel::Overloaded);
	TestTrue(TEXT("Выше 130 %"), GetLevel(39.1f, Max) == ERubezhEncumbranceLevel::Immobile);
	TestTrue(TEXT("Нулевой лимит при грузе"), GetLevel(1.0f, 0.0f) == ERubezhEncumbranceLevel::Immobile);
	TestTrue(TEXT("Нулевой лимит без груза"), GetLevel(0.0f, 0.0f) == ERubezhEncumbranceLevel::Normal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhEncumbranceEffectsTest, "Rubezh.Survival.Encumbrance.Effects", RubezhTestFlags)

bool FRubezhEncumbranceEffectsTest::RunTest(const FString& Parameters)
{
	using Rubezh::Encumbrance::GetEffects;

	const FRubezhEncumbranceEffects Normal = GetEffects(ERubezhEncumbranceLevel::Normal);
	TestEqual(TEXT("Норма: скорость"), Normal.MoveSpeedMultiplier, 1.0f);
	TestTrue(TEXT("Норма: бег"), Normal.bCanSprint);

	const FRubezhEncumbranceEffects Burdened = GetEffects(ERubezhEncumbranceLevel::Burdened);
	TestEqual(TEXT("Нагружен: −15 % скорости"), Burdened.MoveSpeedMultiplier, 0.85f);
	TestEqual(TEXT("Нагружен: выносливость ×1.3"), Burdened.StaminaCostMultiplier, 1.3f);
	TestTrue(TEXT("Нагружен: бег разрешён"), Burdened.bCanSprint);

	const FRubezhEncumbranceEffects Overloaded = GetEffects(ERubezhEncumbranceLevel::Overloaded);
	TestEqual(TEXT("Перегруз: −35 % скорости"), Overloaded.MoveSpeedMultiplier, 0.65f);
	TestEqual(TEXT("Перегруз: выносливость ×2"), Overloaded.StaminaCostMultiplier, 2.0f);
	TestFalse(TEXT("Перегруз: бег запрещён"), Overloaded.bCanSprint);
	TestTrue(TEXT("Перегруз: идти можно"), Overloaded.bCanMove);

	const FRubezhEncumbranceEffects Immobile = GetEffects(ERubezhEncumbranceLevel::Immobile);
	TestFalse(TEXT("Неподъёмно: двигаться нельзя"), Immobile.bCanMove);
	TestEqual(TEXT("Неподъёмно: скорость 0"), Immobile.MoveSpeedMultiplier, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhItemLostOnDeathTest, "Rubezh.Items.LostOnDeath", RubezhTestFlags)

bool FRubezhItemLostOnDeathTest::RunTest(const FString& Parameters)
{
	using Rubezh::Items::IsLostOnDeath;

	TestFalse(TEXT("Экипировка сохраняется"), IsLostOnDeath(ERubezhItemCategory::Equipment));
	TestTrue(TEXT("Расходники теряются"), IsLostOnDeath(ERubezhItemCategory::Consumable));
	TestTrue(TEXT("Хлам теряется"), IsLostOnDeath(ERubezhItemCategory::Junk));
	TestTrue(TEXT("Ресурсы и ценности теряются"), IsLostOnDeath(ERubezhItemCategory::Valuable));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhCombatLogoutLockTest, "Rubezh.Combat.LogoutLock", RubezhTestFlags)

bool FRubezhCombatLogoutLockTest::RunTest(const FString& Parameters)
{
	using Rubezh::Combat::GetRemainingLogoutLock;
	using Rubezh::Combat::LogoutLockSeconds;

	TestEqual(TEXT("Без боя запрета нет"), GetRemainingLogoutLock(100.0, -1.0), 0.0);
	TestEqual(TEXT("Сразу после урона — полный запрет"), GetRemainingLogoutLock(100.0, 100.0), LogoutLockSeconds);
	TestEqual(TEXT("Через 10 с осталось 5 с"), GetRemainingLogoutLock(110.0, 100.0), 5.0);
	TestEqual(TEXT("Ровно через 15 с выйти можно"), GetRemainingLogoutLock(115.0, 100.0), 0.0);
	TestEqual(TEXT("Позже 15 с — без запрета"), GetRemainingLogoutLock(200.0, 100.0), 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhSprintStaminaTest, "Rubezh.Survival.Stamina.Sprint", RubezhTestFlags)

bool FRubezhSprintStaminaTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Stamina;

	TestTrue(TEXT("Полный запас — бежать можно"), CanStartSprint(100.0f, ERubezhEncumbranceLevel::Normal));
	TestTrue(TEXT("Ровно порог — бежать можно"), CanStartSprint(MinStaminaToStartSprint, ERubezhEncumbranceLevel::Normal));
	TestFalse(TEXT("Ниже порога — сначала отдышаться"), CanStartSprint(MinStaminaToStartSprint - 0.1f, ERubezhEncumbranceLevel::Normal));
	TestTrue(TEXT("Нагружен — бежать можно"), CanStartSprint(100.0f, ERubezhEncumbranceLevel::Burdened));
	TestFalse(TEXT("Перегруз — бег запрещён"), CanStartSprint(100.0f, ERubezhEncumbranceLevel::Overloaded));
	TestFalse(TEXT("Неподъёмно — бег запрещён"), CanStartSprint(100.0f, ERubezhEncumbranceLevel::Immobile));

	TestEqual(TEXT("Итоговая трата без нагрузки"), GetSprintNetDrainPerSecond(ERubezhEncumbranceLevel::Normal), SprintNetDrainPerSecond);
	TestEqual(TEXT("Нагружен — трата ×1.3"), GetSprintNetDrainPerSecond(ERubezhEncumbranceLevel::Burdened), SprintNetDrainPerSecond * 1.3f);

	// Восстановление идёт и во время спринта, поэтому за секунду расход + восстановление дают итоговую трату.
	const float NetPerSecond = GetSprintDrainPerTick(ERubezhEncumbranceLevel::Normal) / TickPeriod + RegenPerSecond;
	TestEqual(TEXT("Расход за период компенсирует восстановление"), NetPerSecond, -SprintNetDrainPerSecond, 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
