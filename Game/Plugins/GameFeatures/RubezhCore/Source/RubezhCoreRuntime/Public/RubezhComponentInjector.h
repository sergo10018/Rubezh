// РУБЕЖ: Земля после разлома

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "RubezhComponentInjector.generated.h"

struct FComponentRequestHandle;

/**
 * Добавляет компоненты «Рубежа» к акторам Lyra через менеджер компонентов (как действие Add Components
 * в Experience, но из кода — без правки ассетов). Сейчас: рюкзак игрока на LyraPlayerState.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhComponentInjector : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TArray<TSharedPtr<FComponentRequestHandle>> RequestHandles;
};
