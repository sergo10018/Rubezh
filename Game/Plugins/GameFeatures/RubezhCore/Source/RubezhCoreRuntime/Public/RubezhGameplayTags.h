// РУБЕЖ: Земля после разлома

#pragma once

#include "NativeGameplayTags.h"

namespace RubezhGameplayTags
{
	/** Персонаж бежит спринтом (выдаётся способностью спринта, пока она активна). */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Sprinting);

	/** Способность спринта (по этому тегу её отменяют при переломе и лечении). */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);

	/** Травмы персонажа (зеркало URubezhInjuryComponent для способностей и интерфейса). */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Injury_Bleeding);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Injury_HeavyBleeding);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Injury_LegFracture);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Injury_Pain);

	/** Персонаж лечится: нельзя стрелять и бегать. */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Treating);

	/** Урон от травмы (кровотечения) — по нему такой урон не вызывает новых травм. */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Injury);

	/** SetByCaller: урон и лечение из кода «Рубежа». */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Healing);

	/** Числа на экземпляре оружия Lyra: урон одной пули, эффективная дальность (м), будет ли следующий выстрел осечкой (1 или 0). */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Damage);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_EffectiveRange);
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_NextShotMisfires);

	/** SetByCaller: изменение выносливости за один период эффекта. */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaDelta);
}
