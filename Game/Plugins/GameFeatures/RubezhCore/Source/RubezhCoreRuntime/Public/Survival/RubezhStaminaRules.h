// РУБЕЖ: Земля после разлома

#pragma once

#include "Survival/RubezhEncumbrance.h"

/**
 * Числа выносливости и спринта (docs/gdd/01_Системы_прототипа.md §3). Стартовые значения для плейтеста.
 * Восстановление идёт всегда; во время спринта расход покрывает его, поэтому итоговая трата —
 * SprintNetDrainPerSecond × множитель нагрузки.
 */
namespace Rubezh::Stamina
{
	/** Шаг периодических эффектов выносливости, с. */
	inline constexpr float TickPeriod = 0.1f;

	/** Восстановление стоя и шагом, ед/с. От 0 до 100 — примерно 12 с. */
	inline constexpr float RegenPerSecond = 8.0f;

	/** Итоговая трата в спринте без перегруза, ед/с. Полный запас — примерно 10 с бега. */
	inline constexpr float SprintNetDrainPerSecond = 10.0f;

	/** Ниже этого запаса спринт не начинается: выдохшийся персонаж сначала отдышится. */
	inline constexpr float MinStaminaToStartSprint = 15.0f;

	/** Во сколько раз спринт быстрее обычного бега. */
	inline constexpr float SprintSpeedMultiplier = 1.5f;

	RUBEZHCORERUNTIME_API bool CanStartSprint(float Stamina, ERubezhEncumbranceLevel Level);

	/** Итоговая трата в спринте с учётом нагрузки, ед/с. */
	RUBEZHCORERUNTIME_API float GetSprintNetDrainPerSecond(ERubezhEncumbranceLevel Level);

	/** Сколько отнять за один период эффекта спринта, чтобы вместе с восстановлением получить итоговую трату (отрицательное). */
	RUBEZHCORERUNTIME_API float GetSprintDrainPerTick(ERubezhEncumbranceLevel Level);
}
