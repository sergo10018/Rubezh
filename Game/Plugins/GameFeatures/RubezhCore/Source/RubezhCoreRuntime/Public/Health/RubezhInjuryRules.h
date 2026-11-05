// РУБЕЖ: Земля после разлома
// Правила травм и лечения (docs/gdd/01_Системы_прототипа.md §3). Только математика — проверяется тестами.
// Что лечит каждый предмет и сколько это длится — в data/items.csv (UseSeconds, HealPercent, DelayedHealPercent, DelayedHealSeconds, Cures, PainReliefSeconds).

#pragma once

#include "Health/RubezhInjury.h"
#include "Math/RandomStream.h"

struct FRubezhItemDefinition;

namespace Rubezh::Injuries
{
	// Шансы травм при попадании.
	inline constexpr float BleedingChance = 0.30f;
	inline constexpr float HeavyBleedingChance = 0.10f;	// только от мощного попадания
	inline constexpr float LegFractureChance = 0.15f;	// только при попадании в ногу

	// Мощное попадание («крупный калибр, дробь»): пока своего оружия нет, считаем по урону одного попадания.
	inline constexpr float HeavyHitMinDamage = 25.0f;

	// Падение с высоты больше 4 м ломает ногу.
	inline constexpr float FallFractureHeightCm = 400.0f;

	// Потеря здоровья, ед/с.
	inline constexpr float BleedingHealthPerSecond = 1.0f;
	inline constexpr float HeavyBleedingHealthPerSecond = 3.0f;
	inline constexpr float BleedTickSeconds = 1.0f;

	// Боль держится столько секунд после последней травмы (обезболивающее глушит её на время, указанное у предмета).
	inline constexpr float PainDurationSeconds = 90.0f;

	// Дрожание прицела от боли: размах (градусы) и частота (колебаний в секунду).
	inline constexpr float PainSwayDegrees = 1.2f;
	inline constexpr float PainSwayFrequency = 0.6f;

	/** Что известно о попадании. */
	struct FHitInfo
	{
		float Damage = 0.0f;
		bool bLegHit = false;
	};

	/** Какие травмы получил персонаж от попадания (к любой травме добавляется боль). */
	RUBEZHCORERUNTIME_API FMask RollHitInjuries(const FHitInfo& Hit, FRandomStream& Random);

	/** Кость ноги манекена (бедро, голень, стопа)? */
	RUBEZHCORERUNTIME_API bool IsLegBone(FName BoneName);

	/** Ломает ли ногу падение с такой высоты. */
	RUBEZHCORERUNTIME_API bool IsFallFracture(float FallHeightCm);

	/** Сколько здоровья в секунду отнимают кровотечения. */
	RUBEZHCORERUNTIME_API float GetHealthLossPerSecond(FMask Injuries);

	/** Перевести проценты лечения в единицы здоровья. */
	RUBEZHCORERUNTIME_API float PercentToHealth(float Percent, float MaxHealth);

	/** Есть ли смысл применять предмет: лечит имеющуюся травму, восстанавливает недостающее здоровье или снимает боль. */
	RUBEZHCORERUNTIME_API bool IsUseful(const FRubezhItemDefinition& Item, FMask Injuries, bool bPainRelieved, float Health, float MaxHealth);

	/** Лечебный ли это предмет вообще. */
	RUBEZHCORERUNTIME_API bool IsMedical(const FRubezhItemDefinition& Item);

	/**
	 * Быстрое лечение (клавиша H): что применить сейчас из того, что есть в рюкзаке.
	 * Порядок: сильное кровотечение → кровотечение → перелом → боль → здоровье (меньшая аптечка, если её хватает).
	 * Пусто — лечить нечем или незачем.
	 */
	RUBEZHCORERUNTIME_API FName ChooseQuickTreatment(const TArray<TPair<FName, const FRubezhItemDefinition*>>& Available,
		FMask Injuries, bool bPainRelieved, float Health, float MaxHealth);
}
