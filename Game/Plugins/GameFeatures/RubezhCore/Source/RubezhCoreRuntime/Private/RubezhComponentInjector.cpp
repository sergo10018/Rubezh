// РУБЕЖ: Земля после разлома

#include "RubezhComponentInjector.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Items/RubezhInventoryComponent.h"
#include "Player/LyraPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhComponentInjector)

void URubezhComponentInjector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UGameFrameworkComponentManager>();
	if (UGameFrameworkComponentManager* Manager = GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>())
	{
		// Компонент создаётся и на сервере, и у клиентов с одинаковым именем — так работает его репликация.
		RequestHandles.Add(Manager->AddComponentRequest(ALyraPlayerState::StaticClass(), URubezhInventoryComponent::StaticClass()));
	}
}

void URubezhComponentInjector::Deinitialize()
{
	RequestHandles.Reset();

	Super::Deinitialize();
}
