// РУБЕЖ: Земля после разлома

#include "UI/RubezhPlayerInput.h"

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
#include "Items/RubezhInteractionComponent.h"
#include "NativeGameplayTags.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "UI/RubezhInventoryWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhPlayerInput)

namespace
{
	// Слой Lyra для игровых меню (курсор, ввод не идёт в игру). Тег объявлен в Config/DefaultGameplayTags.ini.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_Layer_GameMenu, "UI.Layer.GameMenu");

	// Выше раскладки Lyra по умолчанию (0), чтобы клавиша не перехватывалась.
	constexpr int32 PlayerMappingPriority = 1;
}

void URubezhPlayerInput::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const URubezhSettings* Settings = GetDefault<URubezhSettings>();
	ToggleInventoryAction = NewObject<UInputAction>(this, TEXT("IA_Rubezh_ToggleInventory"));
	InteractAction = NewObject<UInputAction>(this, TEXT("IA_Rubezh_Interact"));
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Rubezh_Player"));
	Mapping->MapKey(ToggleInventoryAction, Settings->InventoryKey);
	Mapping->MapKey(InteractAction, Settings->InteractKey);

	Collection.InitializeDependency<UGameFrameworkComponentManager>();
	if (UGameFrameworkComponentManager* Manager = GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>())
	{
		PawnExtensionHandle = Manager->AddExtensionHandler(APawn::StaticClass(),
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandlePawnExtension));
	}
}

void URubezhPlayerInput::Deinitialize()
{
	PawnExtensionHandle.Reset();
	Super::Deinitialize();
}

void URubezhPlayerInput::HandlePawnExtension(AActor* Actor, FName EventName)
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
		UE_LOG(LogRubezh, Warning, TEXT("Клавиши Рубежа не подключены: у персонажа нет Enhanced Input"));
		return;
	}

	InputSubsystem->AddMappingContext(Mapping, PlayerMappingPriority);
	InputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ThisClass::HandleToggleInput);
	InputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteractInput);
}

void URubezhPlayerInput::HandleToggleInput()
{
	ToggleInventory(GetGameInstance()->GetFirstLocalPlayerController());
}

void URubezhPlayerInput::HandleInteractInput()
{
	const APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (URubezhInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<URubezhInteractionComponent>() : nullptr)
	{
		Interaction->Interact();
	}
}

void URubezhPlayerInput::ToggleInventory(APlayerController* PlayerController)
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
