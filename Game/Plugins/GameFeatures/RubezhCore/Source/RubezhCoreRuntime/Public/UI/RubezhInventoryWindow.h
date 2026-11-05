// РУБЕЖ: Земля после разлома

#pragma once

#include "CommonActivatableWidget.h"

#include "RubezhInventoryWindow.generated.h"

class UBorder;
class UCanvasPanel;
class UProgressBar;
class URubezhInventoryComponent;
class URubezhInventoryDragOperation;
class USizeBox;
class UTextBlock;

/**
 * Окно рюкзака: сетка с предметами, вес и уровень перегруза.
 * Открывается на слое UI.Layer.GameMenu (курсор включается, игра не получает ввод),
 * закрывается клавишей инвентаря или Esc. Перекладывание — запрос к серверу (ServerMoveEntry),
 * окно перерисовывается, когда сервер прислал новое содержимое.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryWindow : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	URubezhInventoryWindow(const FObjectInitializer& ObjectInitializer);

	void RequestRotate(int32 EntryIndex);
	void HideDropPreview();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UFUNCTION()
	void Refresh();

	void BuildLayout();
	void RebuildCells(FIntPoint GridSize);
	void RefreshWeight(const URubezhInventoryComponent& Inventory);
	void RefreshItems(const URubezhInventoryComponent& Inventory);

	URubezhInventoryComponent* FindInventory() const;

	/** Клетка, куда ляжет левый верхний угол предмета, и можно ли его туда положить. */
	bool EvaluateDrop(const FDragDropEvent& DragDropEvent, const URubezhInventoryDragOperation& Operation, FIntPoint& OutCell) const;

	TWeakObjectPtr<URubezhInventoryComponent> BoundInventory;
	FIntPoint BuiltGridSize = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> WeightBar;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> GridSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DropPreview;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> CellWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> ItemWidgets;
};
