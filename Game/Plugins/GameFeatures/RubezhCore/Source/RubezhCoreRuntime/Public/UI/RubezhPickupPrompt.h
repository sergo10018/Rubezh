// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"

#include "RubezhPickupPrompt.generated.h"

class UBorder;
class UTextBlock;

/** Подсказка над предметом на земле: «E — подобрать: Бинт ×4» или «Нет места в рюкзаке». Собрана кодом. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhPickupPrompt : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetContent(const FText& ItemName, int32 Count, int32 FittingCount);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionText;
};
