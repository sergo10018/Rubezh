// РУБЕЖ: Земля после разлома

#include "Health/RubezhHealthEffects.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "RubezhGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhHealthEffects)

namespace
{
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

URubezhGE_InjuryDamage::URubezhGE_InjuryDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(ULyraHealthSet::GetDamageAttribute(), RubezhGameplayTags::SetByCaller_Damage));
}

URubezhGE_Heal::URubezhGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(ULyraHealthSet::GetHealingAttribute(), RubezhGameplayTags::SetByCaller_Healing));
}
