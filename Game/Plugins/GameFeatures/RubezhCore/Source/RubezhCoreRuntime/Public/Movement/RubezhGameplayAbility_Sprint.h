// РУБЕЖ: Земля после разлома

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"

#include "RubezhGameplayAbility_Sprint.generated.h"

struct FOnAttributeChangeData;

/**
 * Спринт: пока зажата клавиша, персонаж бежит быстрее и тратит выносливость.
 * Кончилась выносливость или отпущена клавиша — спринт заканчивается.
 * Не начинается при запасе ниже Rubezh::Stamina::MinStaminaToStartSprint, при перегрузе, переломе ноги и во время лечения.
 *
 * Скорость меняется и у клиента (предсказание), и на сервере; расход выносливости решает сервер.
 * TODO(Этап 1): перенести флаг спринта в сохранённые движения своего CharacterMovementComponent,
 * чтобы при плохом пинге не было поправок позиции в начале и конце спринта.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhGameplayAbility_Sprint : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	URubezhGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnSprintInputReleased(float TimeHeld);

	void OnStaminaChanged(const FOnAttributeChangeData& ChangeData);

	FActiveGameplayEffectHandle DrainEffectHandle;

	FDelegateHandle StaminaChangedHandle;
};
