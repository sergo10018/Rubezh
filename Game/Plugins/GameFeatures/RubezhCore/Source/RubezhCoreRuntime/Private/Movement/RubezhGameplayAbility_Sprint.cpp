// РУБЕЖ: Земля после разлома

#include "Movement/RubezhGameplayAbility_Sprint.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RubezhGameplayTags.h"
#include "Survival/RubezhStaminaEffects.h"
#include "Survival/RubezhStaminaRules.h"
#include "Survival/RubezhSurvivalSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhGameplayAbility_Sprint)

namespace
{
	const URubezhSurvivalSet* GetSurvivalSet(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		return ASC ? ASC->GetSet<URubezhSurvivalSet>() : nullptr;
	}

	UCharacterMovementComponent* GetMovement(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
		return Character ? Character->GetCharacterMovement() : nullptr;
	}
}

URubezhGameplayAbility_Sprint::URubezhGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = ELyraAbilityActivationPolicy::WhileInputActive;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationOwnedTags.AddTag(RubezhGameplayTags::Status_Sprinting);
}

bool URubezhGameplayAbility_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const URubezhSurvivalSet* SurvivalSet = GetSurvivalSet(ActorInfo);
	return SurvivalSet
		&& GetMovement(ActorInfo)
		&& Rubezh::Stamina::CanStartSprint(SurvivalSet->GetStamina(), SurvivalSet->GetEncumbranceLevel());
}

void URubezhGameplayAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	SetSprintSpeed(ActorInfo, true);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	StaminaChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetStaminaAttribute())
		.AddUObject(this, &ThisClass::OnStaminaChanged);

	if (HasAuthority(&ActivationInfo))
	{
		const URubezhSurvivalSet* SurvivalSet = GetSurvivalSet(ActorInfo);
		const ERubezhEncumbranceLevel Level = SurvivalSet ? SurvivalSet->GetEncumbranceLevel() : ERubezhEncumbranceLevel::Normal;

		FGameplayEffectSpecHandle DrainSpec = MakeOutgoingGameplayEffectSpec(URubezhGE_SprintStaminaDrain::StaticClass(), GetAbilityLevel());
		if (DrainSpec.IsValid())
		{
			DrainSpec.Data->SetSetByCallerMagnitude(RubezhGameplayTags::SetByCaller_StaminaDelta, Rubezh::Stamina::GetSprintDrainPerTick(Level));
			DrainEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpec);
		}
	}

	if (UAbilityTask_WaitInputRelease* WaitRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true))
	{
		WaitRelease->OnRelease.AddDynamic(this, &ThisClass::OnSprintInputReleased);
		WaitRelease->ReadyForActivation();
	}
}

void URubezhGameplayAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive())
	{
		SetSprintSpeed(ActorInfo, false);

		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetStaminaAttribute()).Remove(StaminaChangedHandle);
			if (DrainEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(DrainEffectHandle);
			}
		}
		StaminaChangedHandle.Reset();
		DrainEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void URubezhGameplayAbility_Sprint::OnSprintInputReleased(float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void URubezhGameplayAbility_Sprint::OnStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue <= 0.0f && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void URubezhGameplayAbility_Sprint::SetSprintSpeed(const FGameplayAbilityActorInfo* ActorInfo, bool bSprinting)
{
	UCharacterMovementComponent* Movement = GetMovement(ActorInfo);
	if (!Movement)
	{
		return;
	}

	if (bSprinting)
	{
		SpeedBeforeSprint = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed = SpeedBeforeSprint * Rubezh::Stamina::SprintSpeedMultiplier;
	}
	else if (SpeedBeforeSprint > 0.0f)
	{
		Movement->MaxWalkSpeed = SpeedBeforeSprint;
		SpeedBeforeSprint = 0.0f;
	}
}
