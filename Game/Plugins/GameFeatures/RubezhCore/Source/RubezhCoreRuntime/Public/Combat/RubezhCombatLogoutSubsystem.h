// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"

#include "RubezhCombatLogoutSubsystem.generated.h"

class APlayerState;
struct FLyraVerbMessage;

namespace Rubezh::Combat
{
	/** Сколько секунд после получения или нанесения урона нельзя выйти из игры (docs/gdd/00_Core_loop.md §7). */
	inline constexpr double LogoutLockSeconds = 15.0;

	/** Сколько секунд запрета осталось. LastCombatTime < 0 — персонаж ещё не был в бою. */
	RUBEZHCORERUNTIME_API double GetRemainingLogoutLock(double Now, double LastCombatTime);
}

/**
 * Серверный учёт времени последнего боя для запрета выхода из игры.
 * Слушает сообщения Lyra об уроне и отмечает и атакующего, и цель.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhCombatLogoutSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Можно ли выйти из игры прямо сейчас (без ожидания). */
	UFUNCTION(BlueprintCallable, Category = "Rubezh|Combat")
	bool CanLogoutNow(const APlayerState* PlayerState) const;

	UFUNCTION(BlueprintCallable, Category = "Rubezh|Combat")
	double GetRemainingLogoutLock(const APlayerState* PlayerState) const;

	/** Отметить участие в бою вручную (урон вне системы сообщений Lyra). */
	void MarkInCombat(const APlayerState* PlayerState);

private:
	void OnDamageMessage(FGameplayTag Channel, const FLyraVerbMessage& Payload);

	double GetNow() const;

	TMap<TWeakObjectPtr<const APlayerState>, double> LastCombatTimeByPlayer;

	FGameplayMessageListenerHandle DamageListenerHandle;
};
