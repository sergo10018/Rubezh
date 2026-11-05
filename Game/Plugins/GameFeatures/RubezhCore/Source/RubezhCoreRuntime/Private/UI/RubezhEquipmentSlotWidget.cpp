// РУБЕЖ: Земля после разлома

#include "UI/RubezhEquipmentSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Items/RubezhEquipment.h"
#include "UI/RubezhInventoryItemWidget.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhInventoryWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhEquipmentSlotWidget)

namespace
{
	UTextBlock* MakeSlotText(UWidgetTree& Tree, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(true);
		return Text;
	}
}

void URubezhEquipmentSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace Rubezh::InventoryStyle;

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(EquipSlotWidth);
	Box->SetHeightOverride(EquipSlotHeight);
	WidgetTree->RootWidget = Box;

	Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetPadding(FMargin(6.0f, 4.0f));
	Box->AddChild(Frame);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	SlotText = MakeSlotText(*WidgetTree, ItemNameFontSize, HintText);
	Column->AddChildToVerticalBox(SlotText);
	ItemText = MakeSlotText(*WidgetTree, TextFontSize - 1, FLinearColor::White);
	Column->AddChildToVerticalBox(ItemText);
}

void URubezhEquipmentSlotWidget::Setup(URubezhInventoryWindow* InWindow, ERubezhEquipSlot InEquipSlot)
{
	Window = InWindow;
	EquipSlot = InEquipSlot;
	SlotText->SetText(FText::FromString(Rubezh::Equipment::GetSlotName(EquipSlot)));
	SetItem(NAME_None, nullptr);
}

void URubezhEquipmentSlotWidget::SetItem(FName InItemId, const FRubezhItemDefinition* Item)
{
	using namespace Rubezh::InventoryStyle;

	ItemId = Item ? InItemId : NAME_None;
	Definition = Item ? *Item : FRubezhItemDefinition();
	ItemText->SetText(Item ? Item->DisplayName : FText::FromString(TEXT("—")));
	ResetHighlight();

	const FString Tooltip = Item
		? FString::Printf(TEXT("%s\n%s\n%.2f кг\nПеретащить в рюкзак или двойной клик — снять, за окно — бросить%s"),
			*Item->DisplayName.ToString(), *Rubezh::Equipment::DescribeStats(*Item), Item->WeightKg,
			EquipSlot == ERubezhEquipSlot::Backpack ? TEXT(" (вместе с вещами)") : TEXT(""))
		: FString::Printf(TEXT("%s: пусто. Перетащите сюда предмет или дважды кликните его в рюкзаке"), Rubezh::Equipment::GetSlotName(EquipSlot));
	SetToolTipText(FText::FromString(Tooltip));
}

void URubezhEquipmentSlotWidget::ResetHighlight()
{
	using namespace Rubezh::InventoryStyle;
	Frame->SetBrushColor(ItemId.IsNone() ? EmptyCell : GetCategoryColor(ERubezhItemCategory::Equipment));
}

FReply URubezhEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!Window.IsValid() || ItemId.IsNone())
	{
		return FReply::Unhandled();
	}
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply URubezhEquipmentSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Window.IsValid() && !ItemId.IsNone() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Window->RequestUnequip(EquipSlot, {});
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void URubezhEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	using namespace Rubezh::InventoryStyle;

	FRubezhInventoryEntry Entry;
	Entry.ItemId = ItemId;
	Entry.Count = 1;
	URubezhInventoryItemWidget* DragVisual = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer());
	DragVisual->Setup(nullptr, ERubezhItemSource::Equipment, INDEX_NONE, Entry, Definition);

	URubezhInventoryDragOperation* Operation = NewObject<URubezhInventoryDragOperation>(this);
	Operation->Source = ERubezhItemSource::Equipment;
	Operation->FromSlot = EquipSlot;
	Operation->ItemId = ItemId;
	Operation->DefaultDragVisual = DragVisual;
	Operation->Pivot = EDragPivot::CenterCenter;
	OutOperation = Operation;

	SetRenderOpacity(DraggedItemOpacity);
}

void URubezhEquipmentSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetRenderOpacity(1.0f);
	if (Window.IsValid())
	{
		Window->HideDropPreview();
	}
}

bool URubezhEquipmentSlotWidget::AcceptsDrag(const UDragDropOperation* Operation) const
{
	const URubezhInventoryDragOperation* Drag = Cast<URubezhInventoryDragOperation>(Operation);
	const FRubezhItemDefinition* Item = (Drag && Window.IsValid()) ? Window->FindEntryDefinition(Drag->EntryIndex) : nullptr;
	return Drag && Drag->Source == ERubezhItemSource::Backpack && Item && Item->EquipSlot == EquipSlot;
}

bool URubezhEquipmentSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	using namespace Rubezh::InventoryStyle;
	const URubezhInventoryDragOperation* Drag = Cast<URubezhInventoryDragOperation>(InOperation);
	if (!Drag)
	{
		return false;
	}
	if (Window.IsValid())
	{
		Window->HideDropPreview();
	}
	if (Drag->Source != ERubezhItemSource::Equipment)
	{
		Frame->SetBrushColor(AcceptsDrag(InOperation) ? DropAllowed : DropBlocked);
	}
	return true;
}

void URubezhEquipmentSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ResetHighlight();
}

bool URubezhEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ResetHighlight();
	const URubezhInventoryDragOperation* Drag = Cast<URubezhInventoryDragOperation>(InOperation);
	if (Drag && Window.IsValid() && AcceptsDrag(InOperation))
	{
		Window->RequestEquip(Drag->EntryIndex);
	}
	else if (Window.IsValid())
	{
		Window->RefreshNow();	// вернуть плитке непрозрачность
	}
	// Отпущенное над слотом не выбрасывается на землю, даже если не подошло.
	return Drag != nullptr;
}
