// РУБЕЖ: Земля после разлома

#include "Animals/RubezhAnimalRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags AnimalTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	FRubezhAnimalDefinition MakeTestDog()
	{
		FRubezhAnimalDefinition Dog;
		Dog.BiteRangeCm = 150.0f;
		Dog.LoseRadiusCm = 6000.0f;
		Dog.FleeHealthPercent = 25.0f;
		Dog.PackRoutPercent = 50.0f;
		Dog.RunSpeed = 650.0f;
		return Dog;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhAnimalDecideTest, "Rubezh.Animals.Decide", AnimalTestFlags)
bool FRubezhAnimalDecideTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Animals;
	const FRubezhAnimalDefinition Dog = MakeTestDog();

	FSenses Senses;
	TestTrue(TEXT("Без цели бродит"), Decide(Dog, Senses) == ERubezhAnimalState::Wander);

	Senses.bHasTarget = true;
	Senses.TargetDistanceCm = 2000.0f;
	TestTrue(TEXT("Цель далеко — бежит к ней"), Decide(Dog, Senses) == ERubezhAnimalState::Chase);

	Senses.TargetDistanceCm = 100.0f;
	TestTrue(TEXT("Цель рядом — кусает"), Decide(Dog, Senses) == ERubezhAnimalState::Bite);

	Senses.TargetDistanceCm = 7000.0f;
	TestTrue(TEXT("Цель ушла — бродит"), Decide(Dog, Senses) == ERubezhAnimalState::Wander);

	Senses.TargetDistanceCm = 100.0f;
	Senses.bTargetInSafeZone = true;
	TestTrue(TEXT("В поселении цель потеряна"), Decide(Dog, Senses) == ERubezhAnimalState::Wander);
	Senses.bTargetInSafeZone = false;

	Senses.HealthFraction = 0.2f;
	TestTrue(TEXT("Сильно ранена — бежит"), Decide(Dog, Senses) == ERubezhAnimalState::Flee);
	Senses.HealthFraction = 1.0f;

	Senses.bPackRouted = true;
	TestTrue(TEXT("Стая разбита — бежит"), Decide(Dog, Senses) == ERubezhAnimalState::Flee);
	Senses.bPackRouted = false;

	Senses.HealthFraction = 0.0f;
	TestTrue(TEXT("Нет здоровья — мёртв"), Decide(Dog, Senses) == ERubezhAnimalState::Dead);

	FRubezhAnimalDefinition Boar = MakeTestDog();
	Boar.FleeHealthPercent = 0.0f;
	Senses.HealthFraction = 0.01f;
	TestTrue(TEXT("Кабан бьётся до конца"), Decide(Boar, Senses) == ERubezhAnimalState::Bite);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhAnimalPackTest, "Rubezh.Animals.Pack", AnimalTestFlags)
bool FRubezhAnimalPackTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Animals;
	const FRubezhAnimalDefinition Dog = MakeTestDog();

	TestFalse(TEXT("Целая стая не бежит"), IsPackRouted(Dog, 4, 4));
	TestFalse(TEXT("Одна потеря из 4 — не бежит"), IsPackRouted(Dog, 3, 4));
	TestTrue(TEXT("Половина потеряна — бежит"), IsPackRouted(Dog, 2, 4));
	TestFalse(TEXT("Одна потеря из 5 — не бежит"), IsPackRouted(Dog, 4, 5));
	TestFalse(TEXT("Две из 5 меньше половины"), IsPackRouted(Dog, 3, 5));
	TestTrue(TEXT("Три из 5 больше половины"), IsPackRouted(Dog, 2, 5));
	TestFalse(TEXT("Одиночка стаей не считается"), IsPackRouted(Dog, 0, 1));

	const FVector2D First = GetSurroundOffset(0, 4, 400.0f);
	const FVector2D Opposite = GetSurroundOffset(2, 4, 400.0f);
	TestEqual(TEXT("Звери встают на нужном расстоянии"), First.Size(), 400.0, 0.01);
	TestTrue(TEXT("Двое из четырёх — напротив друг друга"), (First + Opposite).IsNearlyZero(0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhAnimalChargeTest, "Rubezh.Animals.Charge", AnimalTestFlags)
bool FRubezhAnimalChargeTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Animals;
	FRubezhAnimalDefinition Boar;
	Boar.RunSpeed = 500.0f;
	Boar.ChargeStartCm = 1500.0f;
	Boar.ChargeSpeed = 850.0f;

	TestEqual(TEXT("Издалека бежит обычно"), GetChaseSpeed(Boar, 3000.0f), 500.0f);
	TestEqual(TEXT("Вблизи таранит"), GetChaseSpeed(Boar, 1000.0f), 850.0f);
	TestEqual(TEXT("Собака не таранит"), GetChaseSpeed(MakeTestDog(), 100.0f), 650.0f);
	return true;
}

#endif
