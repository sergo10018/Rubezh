// РУБЕЖ: Земля после разлома

#pragma once

#include "Blueprint/UserWidget.h"
#include "Items/RubezhInventoryGrid.h"
#include "UI/RubezhInventoryItemWidget.h"

#include "RubezhSlotGridWidget.generated.h"

class UBorder;
class UCanvasPanel;
class URubezhInventoryWindow;
class USizeBox;

/**
 * Ячейки рюкзака или обыскиваемого ящика рядами по Rubezh::InventoryStyle::SlotColumns, с плитками предметов.
 * Подсвечивает ячейку под перетаскиваемым предметом и передаёт отпускание окну (URubezhInventoryWindow::HandleGridDrop).
 * Собирается кодом, без ассета виджета.
 */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhSlotGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(URubezhInventoryWindow* InWindow, ERubezhItemSource InSource);

	/** Показать содержимое. Lookup — описания предметов. */
	void SetContents(const FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup);

	void HideHighlight();

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void RebuildCells(int32 CellCount);

	/** Ячейка под курсором; INDEX_NONE — курсор между рядами или за последней ячейкой. */
	int32 GetCellUnderCursor(const FDragDropEvent& DragDropEvent) const;

	TWeakObjectPtr<URubezhInventoryWindow> Window;
	ERubezhItemSource Source = ERubezhItemSource::Backpack;
	int32 BuiltSlotCount = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Highlight;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> CellWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> ItemWidgets;
};
