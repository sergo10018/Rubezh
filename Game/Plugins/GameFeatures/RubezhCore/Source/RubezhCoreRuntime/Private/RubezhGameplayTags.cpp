// РУБЕЖ: Земля после разлома

#include "RubezhGameplayTags.h"

namespace RubezhGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sprinting, "Status.Sprinting", "Персонаж бежит спринтом");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Sprint, "Ability.Rubezh.Sprint", "Способность спринта");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Injury_Bleeding, "Status.Injury.Bleeding", "Кровотечение");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Injury_HeavyBleeding, "Status.Injury.HeavyBleeding", "Сильное кровотечение");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Injury_LegFracture, "Status.Injury.LegFracture", "Перелом ноги");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Injury_Pain, "Status.Injury.Pain", "Боль");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Treating, "Status.Treating", "Персонаж лечится");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Injury, "Rubezh.Damage.Injury", "Урон от травмы");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Rubezh.Damage", "Урон");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Healing, "SetByCaller.Rubezh.Healing", "Лечение");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaDelta, "SetByCaller.Rubezh.StaminaDelta", "Изменение выносливости за период эффекта");
}
