// РУБЕЖ: Земля после разлома

#include "Survival/RubezhStaminaEffects.h"

#include "RubezhGameplayTags.h"
#include "Survival/RubezhStaminaRules.h"
#include "Survival/RubezhSurvivalSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhStaminaEffects)

namespace
{
	FGameplayModifierInfo MakeStaminaModifier(const FGameplayEffectModifierMagnitude& Magnitude)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = URubezhSurvivalSet::GetStaminaAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = Magnitude;
		return Modifier;
	}
}

URubezhGE_StaminaRegen::URubezhGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(Rubezh::Stamina::TickPeriod);
	Modifiers.Add(MakeStaminaModifier(FScalableFloat(Rubezh::Stamina::RegenPerSecond * Rubezh::Stamina::TickPeriod)));
}

URubezhGE_SprintStaminaDrain::URubezhGE_SprintStaminaDrain()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(Rubezh::Stamina::TickPeriod);

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = RubezhGameplayTags::SetByCaller_StaminaDelta;
	Modifiers.Add(MakeStaminaModifier(FGameplayEffectModifierMagnitude(SetByCaller)));
}
