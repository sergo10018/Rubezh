// РУБЕЖ: Земля после разлома

#include "RubezhComponentInjector.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Character/LyraCharacter.h"
#include "Health/RubezhInjuryComponent.h"
#include "Items/RubezhInteractionComponent.h"
#include "Items/RubezhInventoryComponent.h"
#include "Player/LyraPlayerState.h"
#include "Settlement/RubezhSafeZoneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhComponentInjector)

void URubezhComponentInjector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UGameFrameworkComponentManager>();
	if (UGameFrameworkComponentManager* Manager = GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>())
	{
		// Компонент создаётся и на сервере, и у клиентов с одинаковым именем — так работает его репликация.
		RequestHandles.Add(Manager->AddComponentRequest(ALyraPlayerState::StaticClass(), URubezhInventoryComponent::StaticClass()));
		RequestHandles.Add(Manager->AddComponentRequest(ALyraCharacter::StaticClass(), URubezhInteractionComponent::StaticClass()));
		RequestHandles.Add(Manager->AddComponentRequest(ALyraCharacter::StaticClass(), URubezhInjuryComponent::StaticClass()));
		RequestHandles.Add(Manager->AddComponentRequest(ALyraCharacter::StaticClass(), URubezhSafeZoneComponent::StaticClass()));
	}
}

void URubezhComponentInjector::Deinitialize()
{
	RequestHandles.Reset();

	Super::Deinitialize();
}
