// РУБЕЖ: Земля после разлома

#include "UI/RubezhSlotGridWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhInventoryWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhSlotGridWidget)

namespace
{
	constexpr int32 CellZOrder = 0;
	constexpr int32 ItemZOrder = 1;
	constexpr int32 HighlightZOrder = 2;

	FVector2D GetCellPosition(int32 Cell)
	{
		using namespace Rubezh::InventoryStyle;
		return FVector2D((Cell % SlotColumns) * CellSize, (Cell / SlotColumns) * CellSize);
	}
}

void URubezhSlotGridWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace Rubezh::InventoryStyle;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(GridBackground);
	Frame->SetPadding(FMargin(CellGap));
	WidgetTree->RootWidget = Frame;

	SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	Frame->SetContent(SizeBox);
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	SizeBox->AddChild(Canvas);

	Highlight = WidgetTree->ConstructWidget<UBorder>();
	Highlight->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* HighlightSlot = Canvas->AddChildToCanvas(Highlight);
	HighlightSlot->SetSize(FVector2D(CellSize - CellGap, CellSize - CellGap));
	HighlightSlot->SetZOrder(HighlightZOrder);
}

void URubezhSlotGridWidget::Setup(URubezhInventoryWindow* InWindow, ERubezhItemSource InSource)
{
	Window = InWindow;
	Source = InSource;
}

void URubezhSlotGridWidget::RebuildCells(int32 CellCount)
{
	using namespace Rubezh::InventoryStyle;

	if (CellCount == BuiltSlotCount)
	{
		return;
	}
	BuiltSlotCount = CellCount;

	for (UWidget* Cell : CellWidgets)
	{
		Cell->RemoveFromParent();
	}
	CellWidgets.Reset();

	const int32 Rows = FMath::Max(1, FMath::DivideAndRoundUp(CellCount, SlotColumns));
	SizeBox->SetWidthOverride(SlotColumns * CellSize - CellGap);
	SizeBox->SetHeightOverride(Rows * CellSize - CellGap);

	for (int32 Index = 0; Index < CellCount; ++Index)
	{
		UBorder* Cell = WidgetTree->ConstructWidget<UBorder>();
		Cell->SetBrushColor(EmptyCell);
		Cell->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* CellSlot = Canvas->AddChildToCanvas(Cell);
		CellSlot->SetPosition(GetCellPosition(Index));
		CellSlot->SetSize(FVector2D(CellSize - CellGap, CellSize - CellGap));
		CellSlot->SetZOrder(CellZOrder);
		CellWidgets.Add(Cell);
	}
}

void URubezhSlotGridWidget::SetContents(const FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup)
{
	RebuildCells(Grid.GetSlotCount());

	for (UWidget* Item : ItemWidgets)
	{
		Item->RemoveFromParent();
	}
	ItemWidgets.Reset();
	HideHighlight();

	const TArray<FRubezhInventoryEntry>& Entries = Grid.GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FRubezhInventoryEntry& Entry = Entries[Index];
		const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
		if (!Definition)
		{
			continue;
		}

		URubezhInventoryItemWidget* Item = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer());
		Item->Setup(Window.Get(), Source, Index, Entry, *Definition);
		UCanvasPanelSlot* ItemSlot = Canvas->AddChildToCanvas(Item);
		ItemSlot->SetPosition(GetCellPosition(Entry.Slot));
		ItemSlot->SetAutoSize(true);
		ItemSlot->SetZOrder(ItemZOrder);
		ItemWidgets.Add(Item);
	}
}

int32 URubezhSlotGridWidget::GetCellUnderCursor(const FDragDropEvent& DragDropEvent) const
{
	using namespace Rubezh::InventoryStyle;

	const FVector2D Local = Canvas->GetCachedGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
	const int32 Column = FMath::FloorToInt(Local.X / CellSize);
	const int32 Row = FMath::FloorToInt(Local.Y / CellSize);
	const int32 Cell = Row * SlotColumns + Column;
	const bool bInside = Local.X >= 0.0f && Local.Y >= 0.0f && Column < SlotColumns;
	return (bInside && Cell < BuiltSlotCount) ? Cell : INDEX_NONE;
}

bool URubezhSlotGridWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	using namespace Rubezh::InventoryStyle;

	if (!Cast<URubezhInventoryDragOperation>(InOperation))
	{
		return false;
	}
	if (Window.IsValid())
	{
		Window->ShowGroundHint(false);
	}

	const int32 Cell = GetCellUnderCursor(InDragDropEvent);
	if (Cell == INDEX_NONE)
	{
		HideHighlight();
		return true;
	}

	Cast<UCanvasPanelSlot>(Highlight->Slot)->SetPosition(GetCellPosition(Cell));
	Highlight->SetBrushColor(DropAllowed);
	Highlight->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

bool URubezhSlotGridWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideHighlight();
	const URubezhInventoryDragOperation* Operation = Cast<URubezhInventoryDragOperation>(InOperation);
	if (!Operation || !Window.IsValid())
	{
		return false;
	}

	// Мимо ячеек (за последней) — кладём «куда влезет».
	Window->HandleGridDrop(Source, GetCellUnderCursor(InDragDropEvent), *Operation);
	return true;
}

void URubezhSlotGridWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideHighlight();
}

void URubezhSlotGridWidget::HideHighlight()
{
	if (Highlight)
	{
		Highlight->SetVisibility(ESlateVisibility::Collapsed);
	}
}
