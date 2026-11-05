// РУБЕЖ: Земля после разлома

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "RubezhInventoryInput.generated.h"

class APlayerController;
class UInputAction;
class UInputMappingContext;
class URubezhInventoryWindow;
struct FComponentRequestHandle;

/**
 * Клавиша открытия рюкзака (URubezhSettings::InventoryKey, по умолчанию I).
 * Действие и раскладка создаются кодом и подключаются по событию Lyra «BindInputsNow» —
 * так же, как это делает действие Game Feature «Add Input Mapping», но без правки ассетов.
 * Lyra очищает раскладки при появлении нового персонажа, поэтому подключение повторяется после каждого возрождения.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryInput : public UGameInstanceSubsystem
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

	TSharedPtr<FComponentRequestHandle> PawnExtensionHandle;
	TWeakObjectPtr<URubezhInventoryWindow> OpenWindow;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;
};
