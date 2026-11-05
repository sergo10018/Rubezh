// РУБЕЖ: Земля после разлома

#include "UI/RubezhInventoryInput.h"

#include "Character/LyraHeroComponent.h"
#include "CommonUIExtensions.h"
#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NativeGameplayTags.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "UI/RubezhInventoryWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryInput)

namespace
{
	// Слой Lyra для игровых меню (курсор, ввод не идёт в игру). Тег объявлен в Config/DefaultGameplayTags.ini.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_Layer_GameMenu, "UI.Layer.GameMenu");

	// Выше раскладки Lyra по умолчанию (0), чтобы клавиша не перехватывалась.
	constexpr int32 InventoryMappingPriority = 1;
}

void URubezhInventoryInput::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ToggleAction = NewObject<UInputAction>(this, TEXT("IA_Rubezh_ToggleInventory"));
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Rubezh_Inventory"));
	Mapping->MapKey(ToggleAction, GetDefault<URubezhSettings>()->InventoryKey);

	Collection.InitializeDependency<UGameFrameworkComponentManager>();
	if (UGameFrameworkComponentManager* Manager = GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>())
	{
		PawnExtensionHandle = Manager->AddExtensionHandler(APawn::StaticClass(),
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandlePawnExtension));
	}
}

void URubezhInventoryInput::Deinitialize()
{
	PawnExtensionHandle.Reset();
	Super::Deinitialize();
}

void URubezhInventoryInput::HandlePawnExtension(AActor* Actor, FName EventName)
{
	if (EventName != ULyraHeroComponent::NAME_BindInputsNow)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(Actor);
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UEnhancedInputComponent* InputComponent = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!InputSubsystem || !InputComponent)
	{
		UE_LOG(LogRubezh, Warning, TEXT("Клавиша рюкзака не подключена: у персонажа нет Enhanced Input"));
		return;
	}

	InputSubsystem->AddMappingContext(Mapping, InventoryMappingPriority);
	InputComponent->BindAction(ToggleAction, ETriggerEvent::Started, this, &ThisClass::HandleToggleInput);
}

void URubezhInventoryInput::HandleToggleInput()
{
	ToggleInventory(GetGameInstance()->GetFirstLocalPlayerController());
}

void URubezhInventoryInput::ToggleInventory(APlayerController* PlayerController)
{
	if (URubezhInventoryWindow* Window = OpenWindow.Get(); Window && Window->IsActivated())
	{
		Window->DeactivateWidget();
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		return;
	}

	OpenWindow = Cast<URubezhInventoryWindow>(UCommonUIExtensions::PushContentToLayer_ForPlayer(
		LocalPlayer, TAG_UI_Layer_GameMenu, URubezhInventoryWindow::StaticClass()));
	if (!OpenWindow.IsValid())
	{
		UE_LOG(LogRubezh, Error, TEXT("Не удалось открыть рюкзак: нет слоя UI.Layer.GameMenu в интерфейсе Lyra"));
	}
}
