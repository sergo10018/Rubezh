// РУБЕЖ: Земля после разлома

#pragma once

#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Survival/RubezhEncumbrance.h"

#include "RubezhSurvivalSet.generated.h"

/**
 * Атрибуты выживания: выносливость и переносимый вес (docs/gdd/01_Системы_прототипа.md §2–3).
 * CarriedWeight выставляет серверный инвентарь; модификаторы GE меняют только лимиты.
 */
UCLASS(BlueprintType)
class RUBEZHCORERUNTIME_API URubezhSurvivalSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:
	URubezhSurvivalSet();

	ATTRIBUTE_ACCESSORS(URubezhSurvivalSet, Stamina);
	ATTRIBUTE_ACCESSORS(URubezhSurvivalSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(URubezhSurvivalSet, CarriedWeight);
	ATTRIBUTE_ACCESSORS(URubezhSurvivalSet, MaxCarryWeight);

	ERubezhEncumbranceLevel GetEncumbranceLevel() const;

protected:
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CarriedWeight(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxCarryWeight(const FGameplayAttributeData& OldValue);

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// Текущая выносливость: тратится на бег, прыжок, удержание прицела винтовки.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Rubezh|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Rubezh|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;

	// Суммарный вес снаряжения и рюкзака, кг. Скрыт от модификаторов: его считает инвентарь.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CarriedWeight, Category = "Rubezh|Survival", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData CarriedWeight;

	// Лимит веса, кг. После прототипа растёт от Силы.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxCarryWeight, Category = "Rubezh|Survival", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxCarryWeight;
};
