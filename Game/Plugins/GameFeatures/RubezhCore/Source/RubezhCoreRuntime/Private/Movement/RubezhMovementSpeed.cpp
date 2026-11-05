// РУБЕЖ: Земля после разлома

#include "Movement/RubezhMovementSpeed.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RubezhGameplayTags.h"
#include "Survival/RubezhStaminaRules.h"
#include "Survival/RubezhSurvivalSet.h"

namespace Rubezh::Movement
{
	float ComputeWalkSpeed(float BaseSpeed, ERubezhEncumbranceLevel Level, bool bSprinting)
	{
		const FRubezhEncumbranceEffects Effects = Encumbrance::GetEffects(Level);
		const float SprintMultiplier = (bSprinting && Effects.bCanSprint) ? Stamina::SprintSpeedMultiplier : 1.0f;
		return BaseSpeed * Effects.MoveSpeedMultiplier * SprintMultiplier;
	}

	void RefreshWalkSpeed(ACharacter* Character)
	{
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
		const URubezhSurvivalSet* SurvivalSet = ASC ? ASC->GetSet<URubezhSurvivalSet>() : nullptr;
		if (!Movement || !SurvivalSet)
		{
			return;
		}

		// База — значение из настроек персонажа (шаблона компонента), а не текущее, иначе множители накапливаются.
		const UCharacterMovementComponent* Archetype = Cast<UCharacterMovementComponent>(Movement->GetArchetype());
		const float BaseSpeed = Archetype ? Archetype->MaxWalkSpeed : Movement->MaxWalkSpeed;
		const float BaseMinAnalogSpeed = Archetype ? Archetype->MinAnalogWalkSpeed : Movement->MinAnalogWalkSpeed;
		const bool bSprinting = ASC->HasMatchingGameplayTag(RubezhGameplayTags::Status_Sprinting);
		const ERubezhEncumbranceLevel Level = SurvivalSet->GetEncumbranceLevel();

		Movement->MaxWalkSpeed = ComputeWalkSpeed(BaseSpeed, Level, bSprinting);
		// MinAnalogWalkSpeed — нижняя граница скорости, она сильнее MaxWalkSpeed. Без её обнуления
		// «неподъёмный» персонаж всё равно идёт со скоростью 200.
		Movement->MinAnalogWalkSpeed = Encumbrance::GetEffects(Level).bCanMove ? BaseMinAnalogSpeed : 0.0f;
	}
}
