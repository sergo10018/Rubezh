// РУБЕЖ: Земля после разлома

#include "Combat/RubezhCombatLogoutSubsystem.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Messages/LyraVerbMessage.h"
#include "Messages/LyraVerbMessageHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhCombatLogoutSubsystem)

namespace Rubezh::Combat
{
	double GetRemainingLogoutLock(double Now, double LastCombatTime)
	{
		if (LastCombatTime < 0.0)
		{
			return 0.0;
		}
		return FMath::Max(0.0, LastCombatTime + LogoutLockSeconds - Now);
	}
}

bool URubezhCombatLogoutSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Решение о выходе принимает сервер; клиентам этот учёт не нужен.
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

void URubezhCombatLogoutSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	DamageListenerHandle = MessageSubsystem.RegisterListener(TAG_Lyra_Damage_Message, this, &ThisClass::OnDamageMessage);
}

void URubezhCombatLogoutSubsystem::Deinitialize()
{
	DamageListenerHandle.Unregister();
	LastCombatTimeByPlayer.Reset();

	Super::Deinitialize();
}

bool URubezhCombatLogoutSubsystem::CanLogoutNow(const APlayerState* PlayerState) const
{
	return GetRemainingLogoutLock(PlayerState) <= 0.0;
}

double URubezhCombatLogoutSubsystem::GetRemainingLogoutLock(const APlayerState* PlayerState) const
{
	const double* LastCombatTime = LastCombatTimeByPlayer.Find(PlayerState);
	return Rubezh::Combat::GetRemainingLogoutLock(GetNow(), LastCombatTime ? *LastCombatTime : -1.0);
}

void URubezhCombatLogoutSubsystem::MarkInCombat(const APlayerState* PlayerState)
{
	if (PlayerState)
	{
		LastCombatTimeByPlayer.Add(PlayerState, GetNow());
	}
}

void URubezhCombatLogoutSubsystem::OnDamageMessage(FGameplayTag Channel, const FLyraVerbMessage& Payload)
{
	// Урон от животных и среды тоже держит в бою: цель отмечается всегда, атакующий — если он игрок.
	MarkInCombat(ULyraVerbMessageHelpers::GetPlayerStateFromObject(Payload.Target));
	MarkInCombat(ULyraVerbMessageHelpers::GetPlayerStateFromObject(Payload.Instigator));
}

double URubezhCombatLogoutSubsystem::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}
