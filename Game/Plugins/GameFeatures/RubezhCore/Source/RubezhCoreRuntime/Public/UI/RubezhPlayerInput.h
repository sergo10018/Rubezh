// РУБЕЖ: Земля после разлома

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "RubezhPlayerInput.generated.h"

class APlayerController;
class UInputAction;
class UInputMappingContext;
class URubezhInventoryWindow;
struct FComponentRequestHandle;

/**
 * Клавиши «Рубежа» поверх раскладки Lyra: рюкзак (URubezhSettings::InventoryKey, по умолчанию I),
 * взаимодействие — подбор предметов (InteractKey, по умолчанию E) и быстрое лечение (HealKey, по умолчанию H).
 * Действия и раскладка создаются кодом и подключаются по событию Lyra «BindInputsNow» —
 * так же, как это делает действие Game Feature «Add Input Mapping», но без правки ассетов.
 * Lyra очищает раскладки при появлении нового персонажа, поэтому подключение повторяется после каждого возрождения.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhPlayerInput : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Открыть окно рюкзака, а если оно открыто — закрыть. */
	void ToggleInventory(APlayerController* PlayerController);

private:
	void HandlePawnExtension(AActor* Actor, FName EventName);
	void HandleToggleInput();
	void HandleInteractInput();
	void HandleHealInput();

	TSharedPtr<FComponentRequestHandle> PawnExtensionHandle;
	TWeakObjectPtr<URubezhInventoryWindow> OpenWindow;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> HealAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;
};
