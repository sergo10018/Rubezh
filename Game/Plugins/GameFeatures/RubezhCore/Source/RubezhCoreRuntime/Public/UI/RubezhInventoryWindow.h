// РУБЕЖ: Земля после разлома

#pragma once

#include "CommonActivatableWidget.h"
#include "Items/RubezhEquipSlot.h"

#include "RubezhInventoryWindow.generated.h"

class UBorder;
class UCanvasPanel;
class UProgressBar;
class URubezhEquipmentSlotWidget;
struct FRubezhItemDefinition;
class URubezhInventoryComponent;
class URubezhInventoryDragOperation;
class USizeBox;
class UTextBlock;

/**
 * Окно рюкзака: слева снаряжение (голова, бронежилет, одежда, рюкзак) и суммарная защита,
 * справа сетка с предметами, вес и уровень перегруза.
 * Открывается на слое UI.Layer.GameMenu (курсор включается, игра не получает ввод),
 * закрывается клавишей инвентаря или Esc. Перекладывание — запрос к серверу (ServerMoveEntry),
 * предмет, отпущенный за пределами сетки, выбрасывается на землю (ServerDropEntry).
 * Окно перерисовывается, когда сервер прислал новое содержимое.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhInventoryWindow : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	URubezhInventoryWindow(const FObjectInitializer& ObjectInitializer);

	void RequestRotate(int32 EntryIndex);
	void RequestUse(int32 EntryIndex);
	void RequestEquip(int32 EntryIndex);
	/** Снять в сетку: на клетку Cell или (без неё) куда влезет. */
	void RequestUnequip(ERubezhEquipSlot EquipSlot, TOptional<FIntPoint> Cell, bool bRotated);
	void HideDropPreview();
	void RefreshNow() { Refresh(); }

	const FRubezhItemDefinition* FindEntryDefinition(int32 EntryIndex) const;

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
	void RefreshEquipment(const URubezhInventoryComponent& Inventory);
	class UWidget* BuildEquipmentColumn();

	/** Показать причину отказа красным в строке подсказки на несколько секунд. */
	void ShowError(const FString& Message);
	void ClearError();
	bool bShowingError = false;
	FTimerHandle ErrorTimer;

	/** Курсор над окном? Отпущенное за окном выбрасывается на землю. */
	bool IsOverWindow(const FDragDropEvent& DragDropEvent) const;

	URubezhInventoryComponent* FindInventory() const;

	/** Курсор над сеткой? */
	bool IsOverGrid(const FDragDropEvent& DragDropEvent) const;
	void ShowDropToGroundHint(bool bShow);

	/** Клетка, куда ляжет левый верхний угол предмета, и можно ли его туда положить. */
	bool EvaluateDrop(const FDragDropEvent& DragDropEvent, const URubezhInventoryDragOperation& Operation, FIntPoint& OutCell) const;

	TWeakObjectPtr<URubezhInventoryComponent> BoundInventory;
	FIntPoint BuiltGridSize = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HintLine;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeightText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProtectionText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> WindowPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URubezhEquipmentSlotWidget>> EquipSlots;

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
