// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"

#include "RubezhPickupPrompt.generated.h"

class UBorder;
class UTextBlock;

/** Подсказка под прицелом: название предмета или ящика и действие («E — подобрать», «Обыск… 1,5 с»). Собрана кодом. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhPickupPrompt : public UUserWidget
{
	GENERATED_BODY()

public:
	/** bWarning — действие невозможно (красным). */
	void SetMessage(const FText& Name, const FString& Action, bool bWarning);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionText;
};
