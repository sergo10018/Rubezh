// РУБЕЖ: Земля после разлома

#include "Survival/RubezhSurvivalSet.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhSurvivalSet)

namespace
{
	constexpr float DefaultMaxStamina = 100.0f;
}

URubezhSurvivalSet::URubezhSurvivalSet()
	: Stamina(DefaultMaxStamina)
	, MaxStamina(DefaultMaxStamina)
	, CarriedWeight(0.0f)
	, MaxCarryWeight(Rubezh::Encumbrance::BaseMaxCarryWeight)
{
}

void URubezhSurvivalSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Чужая выносливость и вес — информация, которую соперник не должен видеть.
	DOREPLIFETIME_CONDITION_NOTIFY(URubezhSurvivalSet, Stamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URubezhSurvivalSet, MaxStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URubezhSurvivalSet, CarriedWeight, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(URubezhSurvivalSet, MaxCarryWeight, COND_OwnerOnly, REPNOTIFY_Always);
}

ERubezhEncumbranceLevel URubezhSurvivalSet::GetEncumbranceLevel() const
{
	return Rubezh::Encumbrance::GetLevel(GetCarriedWeight(), GetMaxCarryWeight());
}

void URubezhSurvivalSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URubezhSurvivalSet, Stamina, OldValue);
}

void URubezhSurvivalSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URubezhSurvivalSet, MaxStamina, OldValue);
}

void URubezhSurvivalSet::OnRep_CarriedWeight(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URubezhSurvivalSet, CarriedWeight, OldValue);
}

void URubezhSurvivalSet::OnRep_MaxCarryWeight(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URubezhSurvivalSet, MaxCarryWeight, OldValue);
}

void URubezhSurvivalSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void URubezhSurvivalSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void URubezhSurvivalSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Выносливость не может остаться выше уменьшенного максимума.
	if (Attribute == GetMaxStaminaAttribute() && GetStamina() > NewValue)
	{
		ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent();
		check(LyraASC);

		LyraASC->ApplyModToAttribute(GetStaminaAttribute(), EGameplayModOp::Override, NewValue);
	}
}

void URubezhSurvivalSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute() || Attribute == GetMaxCarryWeightAttribute())
	{
		// Нулевой лимит сломал бы расчёт нагрузки и трату выносливости.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetCarriedWeightAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}
