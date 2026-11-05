// РУБЕЖ: Земля после разлома

#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhItemDefinition.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags InjuryTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::EngineFilter;
	constexpr int32 RollCount = 20000;

	FRubezhItemDefinition MakeMedical(float UseSeconds, float HealPercent, ERubezhInjury Cures, float PainRelief = 0.0f, float DelayedPercent = 0.0f)
	{
		FRubezhItemDefinition Item;
		Item.Category = ERubezhItemCategory::Consumable;
		Item.UseSeconds = UseSeconds;
		Item.HealPercent = HealPercent;
		Item.DelayedHealPercent = DelayedPercent;
		Item.DelayedHealSeconds = DelayedPercent > 0.0f ? 5.0f : 0.0f;
		Item.Cures = Cures;
		Item.PainReliefSeconds = PainRelief;
		return Item;
	}

	/** Аптечка как в data/items.csv. */
	struct FMedicalCatalog
	{
		FRubezhItemDefinition Bandage = MakeMedical(3.0f, 0.0f, ERubezhInjury::Bleeding);
		FRubezhItemDefinition Tourniquet = MakeMedical(4.0f, 0.0f, ERubezhInjury::HeavyBleeding);
		FRubezhItemDefinition Splint = MakeMedical(6.0f, 0.0f, ERubezhInjury::LegFracture);
		FRubezhItemDefinition Painkillers = MakeMedical(2.0f, 0.0f, ERubezhInjury::None, 120.0f);
		FRubezhItemDefinition MedkitSmall = MakeMedical(4.0f, 35.0f, ERubezhInjury::None, 0.0f, 20.0f);	// всего 55 %
		FRubezhItemDefinition MedkitArmy = MakeMedical(7.0f, 50.0f, ERubezhInjury::None, 0.0f, 30.0f);	// всего 80 %
		FRubezhItemDefinition Scrap;	// не медицина

		TArray<TPair<FName, const FRubezhItemDefinition*>> All() const
		{
			return {
				{ TEXT("scrap"), &Scrap }, { TEXT("medkit_army"), &MedkitArmy }, { TEXT("medkit_small"), &MedkitSmall },
				{ TEXT("painkillers"), &Painkillers }, { TEXT("splint"), &Splint }, { TEXT("bandage"), &Bandage },
				{ TEXT("tourniquet"), &Tourniquet } };
		}
	};

	float Frequency(const Rubezh::Injuries::FHitInfo& Hit, ERubezhInjury Injury)
	{
		FRandomStream Random(12345);
		int32 Count = 0;
		for (int32 Index = 0; Index < RollCount; ++Index)
		{
			Count += Rubezh::Injuries::Has(Rubezh::Injuries::RollHitInjuries(Hit, Random), Injury) ? 1 : 0;
		}
		return static_cast<float>(Count) / RollCount;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInjuryHitTest, "Rubezh.Health.Injuries.Hit", InjuryTestFlags)

bool FRubezhInjuryHitTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Injuries;
	const FHitInfo Light{ 15.0f, false };
	const FHitInfo HeavyLeg{ 40.0f, true };

	TestEqual(TEXT("Кровотечение примерно в 30 % попаданий"), Frequency(Light, ERubezhInjury::Bleeding), BleedingChance, 0.02f);
	TestEqual(TEXT("Слабое попадание не даёт сильного кровотечения"), Frequency(Light, ERubezhInjury::HeavyBleeding), 0.0f);
	TestEqual(TEXT("Мощное попадание — сильное кровотечение примерно в 10 %"), Frequency(HeavyLeg, ERubezhInjury::HeavyBleeding), HeavyBleedingChance, 0.02f);
	TestEqual(TEXT("Попадание не в ногу не ломает ногу"), Frequency(Light, ERubezhInjury::LegFracture), 0.0f);
	TestEqual(TEXT("Попадание в ногу — перелом примерно в 15 %"), Frequency(HeavyLeg, ERubezhInjury::LegFracture), LegFractureChance, 0.02f);

	FRandomStream Random(7);
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		const FMask Injuries = RollHitInjuries(HeavyLeg, Random);
		if (Injuries != 0 && !Has(Injuries, ERubezhInjury::Pain))
		{
			AddError(TEXT("Травма без боли"));
			break;
		}
	}
	TestEqual(TEXT("Нулевой урон — без травм"), RollHitInjuries(FHitInfo{ 0.0f, true }, Random), FMask(0));

	TestTrue(TEXT("Голень — нога"), IsLegBone(TEXT("calf_l")));
	TestTrue(TEXT("Бедро — нога"), IsLegBone(TEXT("thigh_r")));
	TestFalse(TEXT("Голова — не нога"), IsLegBone(TEXT("head")));

	TestFalse(TEXT("Прыжок с 3 м — без перелома"), IsFallFracture(300.0f));
	TestTrue(TEXT("Падение с 5 м — перелом"), IsFallFracture(500.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhInjuryBleedTest, "Rubezh.Health.Injuries.Bleeding", InjuryTestFlags)

bool FRubezhInjuryBleedTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Injuries;
	TestEqual(TEXT("Без травм здоровье не уходит"), GetHealthLossPerSecond(0), 0.0f);
	TestEqual(TEXT("Кровотечение — 1 ед/с"), GetHealthLossPerSecond(Bit(ERubezhInjury::Bleeding)), 1.0f);
	TestEqual(TEXT("Оба кровотечения — 4 ед/с"),
		GetHealthLossPerSecond(Bit(ERubezhInjury::Bleeding) | Bit(ERubezhInjury::HeavyBleeding) | Bit(ERubezhInjury::Pain)), 4.0f);
	TestEqual(TEXT("Перелом не отнимает здоровье"), GetHealthLossPerSecond(Bit(ERubezhInjury::LegFracture)), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRubezhTreatmentTest, "Rubezh.Health.Treatment", InjuryTestFlags)

bool FRubezhTreatmentTest::RunTest(const FString& Parameters)
{
	using namespace Rubezh::Injuries;
	const FMedicalCatalog Items;
	const FMask Bleeding = Bit(ERubezhInjury::Bleeding) | Bit(ERubezhInjury::Pain);

	TestTrue(TEXT("Бинт полезен при кровотечении"), IsUseful(Items.Bandage, Bleeding, false, 80.0f, 100.0f));
	TestFalse(TEXT("Бинт бесполезен без кровотечения"), IsUseful(Items.Bandage, 0, false, 80.0f, 100.0f));
	TestFalse(TEXT("Аптечка бесполезна при полном здоровье"), IsUseful(Items.MedkitSmall, Bleeding, false, 100.0f, 100.0f));
	TestTrue(TEXT("Обезболивающее полезно при боли"), IsUseful(Items.Painkillers, Bleeding, false, 100.0f, 100.0f));
	TestFalse(TEXT("…но не второй раз подряд"), IsUseful(Items.Painkillers, Bleeding, true, 100.0f, 100.0f));
	TestFalse(TEXT("Металлолом — не лекарство"), IsUseful(Items.Scrap, Bleeding, false, 10.0f, 100.0f));

	const auto All = Items.All();
	const FMask Everything = Bit(ERubezhInjury::Bleeding) | Bit(ERubezhInjury::HeavyBleeding) | Bit(ERubezhInjury::LegFracture) | Bit(ERubezhInjury::Pain);
	TestEqual(TEXT("Сначала жгут на сильное кровотечение"), ChooseQuickTreatment(All, Everything, false, 50.0f, 100.0f), FName(TEXT("tourniquet")));
	TestEqual(TEXT("Потом бинт"), ChooseQuickTreatment(All, Bleeding | Bit(ERubezhInjury::LegFracture), false, 50.0f, 100.0f), FName(TEXT("bandage")));
	TestEqual(TEXT("Потом шина"), ChooseQuickTreatment(All, Bit(ERubezhInjury::LegFracture) | Bit(ERubezhInjury::Pain), false, 50.0f, 100.0f), FName(TEXT("splint")));
	TestEqual(TEXT("Потом обезболивающее"), ChooseQuickTreatment(All, Bit(ERubezhInjury::Pain), false, 50.0f, 100.0f), FName(TEXT("painkillers")));
	TestEqual(TEXT("Не хватает 50 — малой аптечки (55 %) достаточно"), ChooseQuickTreatment(All, 0, false, 50.0f, 100.0f), FName(TEXT("medkit_small")));
	TestEqual(TEXT("Не хватает 70 — армейская"), ChooseQuickTreatment(All, 0, false, 30.0f, 100.0f), FName(TEXT("medkit_army")));
	TestEqual(TEXT("Проценты считаются от максимума: при максимуме 200 малой (110) не хватит на 120"),
		ChooseQuickTreatment(All, 0, false, 80.0f, 200.0f), FName(TEXT("medkit_army")));
	TestEqual(TEXT("35 % от 100 — 35 здоровья"), PercentToHealth(35.0f, 100.0f), 35.0f);
	TestEqual(TEXT("20 % от 150 — 30 здоровья"), PercentToHealth(20.0f, 150.0f), 30.0f);
	TestTrue(TEXT("Предмет только с отложенным лечением — тоже медицина"), IsMedical(MakeMedical(3.0f, 0.0f, ERubezhInjury::None, 0.0f, 10.0f)));
	TestEqual(TEXT("Здоров — лечить нечего"), ChooseQuickTreatment(All, 0, false, 100.0f, 100.0f), FName(NAME_None));

	const TArray<TPair<FName, const FRubezhItemDefinition*>> OnlySmall = { { TEXT("medkit_small"), &Items.MedkitSmall } };
	TestEqual(TEXT("Кровотечение, а бинта нет — хотя бы аптечка"), ChooseQuickTreatment(OnlySmall, Bleeding, true, 60.0f, 100.0f), FName(TEXT("medkit_small")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
