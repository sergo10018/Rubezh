// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"
#include "Health/RubezhInjury.h"

#include "RubezhHealthStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** Строка состояния над полоской здоровья: травмы, ход лечения и подсказка клавиши быстрого лечения. Собрана кодом. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhHealthStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatus(uint8 Injuries, bool bPainRelieved, const FText& TreatmentName, float TreatmentProgress);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InjuryText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> TreatmentRow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TreatmentText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> TreatmentBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintLine;
};
