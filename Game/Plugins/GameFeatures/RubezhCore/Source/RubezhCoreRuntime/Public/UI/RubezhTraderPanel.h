// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"
#include "Items/RubezhTraderKind.h"

#include "RubezhTraderPanel.generated.h"

class UScrollBox;
class UTextBlock;
class URubezhInventoryComponent;

/** Строка торговца: подпись и одна-две кнопки. Собрана кодом, как и всё окно. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhTraderRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FString& Label, const FString& PrimaryLabel, TFunction<void()> InPrimary, const FString& SecondaryLabel = FString(), TFunction<void()> InSecondary = nullptr);

private:
	UFUNCTION()
	void HandlePrimaryClicked();

	UFUNCTION()
	void HandleSecondaryClicked();

	TFunction<void()> PrimaryAction;
	TFunction<void()> SecondaryAction;
};

/**
 * Колонка торговца в окне рюкзака: что он продаёт, что купит из рюкзака и (у оружейника) что можно починить.
 * Панель только показывает и просит сервер; отказ объясняет сразу, проверив теми же правилами на копии.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhTraderPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh(URubezhInventoryComponent& Inventory);

protected:
	virtual void NativeOnInitialized() override;

private:
	void AddHeader(const FString& Text);
	void AddBuyRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind);
	void AddSellRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind);
	void AddRepairRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind);
	void AddRow(const FString& Label, const FString& PrimaryLabel, TFunction<void()> Primary, const FString& SecondaryLabel = FString(), TFunction<void()> Secondary = nullptr);

	void RequestBuy(URubezhInventoryComponent& Inventory, FName ItemId, int32 Count, ERubezhTraderKind Kind);

	void ShowError(const FString& Message);
	void ClearError();

	FTimerHandle ErrorTimer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ErrorText;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> Rows;
};
