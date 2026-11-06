// РУБЕЖ: Земля после разлома

#include "UI/RubezhEquipmentSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhWeaponRules.h"
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

void URubezhEquipmentSlotWidget::SetupWeapon(URubezhInventoryWindow* InWindow, int32 InWeaponSlot)
{
	Window = InWindow;
	WeaponSlot = InWeaponSlot;
	SlotText->SetText(FText::FromString(FString::Printf(TEXT("%d — %s"), WeaponSlot + 1, Rubezh::Weapons::GetSlotName(WeaponSlot))));
	SetWeapon(FRubezhInventoryEntry(), nullptr);
}

void URubezhEquipmentSlotWidget::SetWeapon(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition* Item)
{
	WeaponEntry = Entry;
	ItemId = Item ? Entry.ItemId : NAME_None;
	Definition = Item ? *Item : FRubezhItemDefinition();
	ItemText->SetText(Item
		? FText::FromString(FString::Printf(TEXT("%s · %d пат. · %.0f %%"), *Item->DisplayName.ToString(), Entry.Rounds, Entry.Durability))
		: FText::FromString(TEXT("—")));
	ResetHighlight();

	SetToolTipText(FText::FromString(Item
		? FString::Printf(TEXT("%s\nЗаряжено: %d, прочность: %.0f %%\nПеретащить в рюкзак или двойной клик — убрать, за окно — бросить"), *Item->DisplayName.ToString(), Entry.Rounds, Entry.Durability)
		: FString::Printf(TEXT("%s: пусто. Перетащите сюда оружие или дважды кликните его в рюкзаке"), Rubezh::Weapons::GetSlotName(WeaponSlot))));
}

void URubezhEquipmentSlotWidget::SetItem(FName InItemId, const FRubezhItemDefinition* Item, float Durability)
{
	using namespace Rubezh::InventoryStyle;

	ItemId = Item ? InItemId : NAME_None;
	Definition = Item ? *Item : FRubezhItemDefinition();
	const bool bShowsWear = Item && Rubezh::Equipment::CanWear(*Item);
	ItemText->SetText(!Item ? FText::FromString(TEXT("—"))
		: bShowsWear ? FText::FromString(FString::Printf(TEXT("%s · %.0f %%"), *Item->DisplayName.ToString(), Durability))
		: Item->DisplayName);
	ResetHighlight();

	const FString Wear = bShowsWear ? FString::Printf(TEXT("\nПрочность: %.0f %%, защита падает вместе с ней"), Durability) : FString();
	const FString Tooltip = Item
		? FString::Printf(TEXT("%s\n%s%s\n%.2f кг\nПеретащить в рюкзак или двойной клик — снять, за окно — бросить%s"),
			*Item->DisplayName.ToString(), *Rubezh::Equipment::DescribeStats(*Item), *Wear, Item->WeightKg,
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
		if (WeaponSlot != INDEX_NONE)
		{
			Window->RequestUnequipWeapon(WeaponSlot, {});
		}
		else
		{
			Window->RequestUnequip(EquipSlot, {});
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void URubezhEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	using namespace Rubezh::InventoryStyle;

	FRubezhInventoryEntry Entry = WeaponSlot != INDEX_NONE ? WeaponEntry : FRubezhInventoryEntry();
	Entry.ItemId = ItemId;
	Entry.Count = 1;
	URubezhInventoryItemWidget* DragVisual = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer());
	DragVisual->Setup(nullptr, ERubezhItemSource::Equipment, INDEX_NONE, Entry, Definition);

	URubezhInventoryDragOperation* Operation = NewObject<URubezhInventoryDragOperation>(this);
	Operation->Source = WeaponSlot != INDEX_NONE ? ERubezhItemSource::Weapon : ERubezhItemSource::Equipment;
	Operation->FromSlot = EquipSlot;
	Operation->FromWeaponSlot = WeaponSlot;
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
	if (!Drag || Drag->Source != ERubezhItemSource::Backpack || !Item)
	{
		return false;
	}
	if (WeaponSlot != INDEX_NONE)
	{
		const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(Drag->ItemId);
		return Weapon && Rubezh::Weapons::SlotAccepts(Weapon->WeaponClass, WeaponSlot);
	}
	return Item->EquipSlot == EquipSlot;
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
	if (Drag->Source == ERubezhItemSource::Backpack || Drag->Source == ERubezhItemSource::Loot)
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
		if (WeaponSlot != INDEX_NONE)
		{
			Window->RequestEquipWeapon(Drag->EntryIndex, WeaponSlot);
		}
		else
		{
			Window->RequestEquip(Drag->EntryIndex);
		}
	}
	else if (Window.IsValid())
	{
		Window->RefreshNow();	// вернуть плитке непрозрачность
	}
	// Отпущенное над слотом не выбрасывается на землю, даже если не подошло.
	return Drag != nullptr;
}
