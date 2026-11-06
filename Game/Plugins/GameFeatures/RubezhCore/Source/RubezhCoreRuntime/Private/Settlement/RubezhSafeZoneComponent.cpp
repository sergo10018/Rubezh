// РУБЕЖ: Земля после разлома

#include "Settlement/RubezhSafeZoneComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "Items/RubezhInteractionComponent.h"
#include "Settlement/RubezhSafeZone.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhSafeZoneComponent)

namespace
{
	// Граница проверяется несколько раз в секунду: дёшево, а задержку не заметить.
	constexpr float CheckIntervalSeconds = 0.25f;

	const FGameplayTagContainer& GetBlockedAbilities()
	{
		static const FGameplayTagContainer Tags = []
		{
			FGameplayTagContainer Result;
			for (const TCHAR* Name : { TEXT("Ability.Type.Action.WeaponFire"), TEXT("Ability.Type.Action.Grenade"), TEXT("Ability.Type.Action.Melee") })
			{
				Result.AddTag(FGameplayTag::RequestGameplayTag(Name, false));
			}
			return Result;
		}();
		return Tags;
	}
}

URubezhSafeZoneComponent::URubezhSafeZoneComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = CheckIntervalSeconds;
}

bool URubezhSafeZoneComponent::IsLocalPlayerPawn() const
{
	const APawn* Pawn = GetPawn<APawn>();
	return Pawn && Pawn->IsLocallyControlled() && Pawn->IsPlayerControlled();
}

void URubezhSafeZoneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = GetPawn<APawn>();
	// Чужим персонажам на клиенте запрет не нужен.
	if (Pawn && (Pawn->HasAuthority() || Pawn->IsLocallyControlled()))
	{
		SetInside(ARubezhSafeZone::IsInsideAny(GetWorld(), Pawn->GetActorLocation()));
	}
}

void URubezhSafeZoneComponent::SetInside(bool bNowInside)
{
	if (bNowInside == bInside)
	{
		return;
	}

	UAbilitySystemComponent* ASC = bNowInside ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn<APawn>()) : BlockedAbilitySystem.Get();
	if (bNowInside && !ASC)
	{
		return;	// способности ещё не выданы — попробуем при следующей проверке
	}

	bInside = bNowInside;
	if (bNowInside)
	{
		BlockedAbilitySystem = ASC;
		ASC->BlockAbilitiesWithTags(GetBlockedAbilities());
		ASC->CancelAbilities(&GetBlockedAbilities());
	}
	else if (ASC)
	{
		ASC->UnBlockAbilitiesWithTags(GetBlockedAbilities());
		BlockedAbilitySystem.Reset();
	}

	if (IsLocalPlayerPawn())
	{
		if (URubezhInteractionComponent* Interaction = GetPawn<APawn>()->FindComponentByClass<URubezhInteractionComponent>())
		{
			Interaction->ShowNotice(bNowInside ? TEXT("Поселение: стрельба запрещена") : TEXT("Вы покинули поселение"));
		}
	}
}

void URubezhSafeZoneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = BlockedAbilitySystem.Get())
	{
		ASC->UnBlockAbilitiesWithTags(GetBlockedAbilities());
	}
	BlockedAbilitySystem.Reset();
	bInside = false;

	Super::EndPlay(EndPlayReason);
}
