// РУБЕЖ: Земля после разлома

#include "UI/RubezhInventoryItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhWeaponDefinition.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhInventoryWindow.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryItemWidget)

namespace
{
	UTextBlock* MakeItemText(UWidgetTree& Tree, int32 FontSize)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor::Black);
		return Text;
	}
}

void URubezhInventoryItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	using namespace Rubezh::InventoryStyle;

	SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(CellSize - CellGap);
	SizeBox->SetHeightOverride(CellSize - CellGap);
	WidgetTree->RootWidget = SizeBox;

	Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetPadding(FMargin(3.0f, 2.0f));
	SizeBox->AddChild(Frame);

	UOverlay* Layout = WidgetTree->ConstructWidget<UOverlay>();
	Frame->SetContent(Layout);

	NameText = MakeItemText(*WidgetTree, ItemNameFontSize);
	NameText->SetAutoWrapText(true);
	UOverlaySlot* NameSlot = Layout->AddChildToOverlay(NameText);
	NameSlot->SetHorizontalAlignment(HAlign_Fill);
	NameSlot->SetVerticalAlignment(VAlign_Top);

	CountText = MakeItemText(*WidgetTree, ItemCountFontSize);
	UOverlaySlot* CountSlot = Layout->AddChildToOverlay(CountText);
	CountSlot->SetHorizontalAlignment(HAlign_Right);
	CountSlot->SetVerticalAlignment(VAlign_Bottom);
}

void URubezhInventoryItemWidget::Setup(URubezhInventoryWindow* InWindow, ERubezhItemSource InSource, int32 InEntryIndex,
	const FRubezhInventoryEntry& InEntry, const FRubezhItemDefinition& InDefinition)
{
	using namespace Rubezh::InventoryStyle;

	Window = InWindow;
	Source = InSource;
	EntryIndex = InEntryIndex;
	Entry = InEntry;
	Definition = InDefinition;

	Frame->SetBrushColor(GetCategoryColor(Definition.Category));
	NameText->SetText(Definition.DisplayName);
	// У магазина внизу — патроны, у оружия — прочность, у стопки — количество.
	if (Definition.MagazineCapacity > 0)
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Entry.Rounds, Definition.MagazineCapacity)));
	}
	else if (Rubezh::Items::FindWeapon(Entry.ItemId) || Rubezh::Equipment::CanWear(Definition))
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%.0f %%"), Entry.Durability)));
	}
	else
	{
		CountText->SetText(Entry.Count > 1 ? FText::AsNumber(Entry.Count) : FText::GetEmpty());
	}
	SetToolTipText(BuildTooltip());
	SetVisibility(InWindow ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

FText URubezhInventoryItemWidget::BuildTooltip() const
{
	const float StackWeight = Definition.WeightKg * Entry.Count;
	const FString Stats = Rubezh::Equipment::DescribeStats(Definition);
	const bool bLootOpen = Window.IsValid() && Window->IsLootOpen();

	TArray<FString> Hints;
	if (Source == ERubezhItemSource::Loot)
	{
		Hints.Add(TEXT("Двойной клик или ПКМ — забрать"));
	}
	else
	{
		if (Rubezh::Items::FindWeapon(Entry.ItemId))
		{
			Hints.Add(TEXT("Двойной клик или перетащить в слот оружия — взять"));
		}
		else if (Definition.MagazineCapacity > 0)
		{
			Hints.Add(TEXT("Перетащите на него патроны — набить, двойной клик — разрядить"));
		}
		else if (Rubezh::Injuries::IsMedical(Definition))
		{
			Hints.Add(TEXT("Двойной клик — применить"));
		}
		else if (Definition.EquipSlot != ERubezhEquipSlot::None)
		{
			Hints.Add(FString::Printf(TEXT("Двойной клик или перетащить в слот «%s» — надеть"), Rubezh::Equipment::GetSlotName(Definition.EquipSlot)));
		}
		if (bLootOpen)
		{
			Hints.Add(TEXT("ПКМ — положить в обыскиваемое"));
		}
	}

	FString State;
	if (Definition.MagazineCapacity > 0)
	{
		State = FString::Printf(TEXT("Патронов: %d из %d\n"), Entry.Rounds, Definition.MagazineCapacity);
	}
	else if (Rubezh::Items::FindWeapon(Entry.ItemId))
	{
		State = FString::Printf(TEXT("Прочность: %.0f %%, заряжено: %d\n"), Entry.Durability, Entry.Rounds);
	}
	else if (Rubezh::Equipment::CanWear(Definition))
	{
		State = FString::Printf(TEXT("Прочность: %.0f %%\n"), Entry.Durability);
	}

	return FText::FromString(FString::Printf(TEXT("%s\n%s%s%d шт. × %.3g кг = %.2f кг\n%s"),
		*Definition.DisplayName.ToString(), Stats.IsEmpty() ? TEXT("") : *(Stats + TEXT("\n")), *State,
		Entry.Count, Definition.WeightKg, StackWeight, *FString::Join(Hints, TEXT("\n"))));
}

FReply URubezhInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!Window.IsValid())
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Window->QuickMove(Source, EntryIndex);
		return FReply::Handled();
	}
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply URubezhInventoryItemWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Window.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Window->Activate(Source, EntryIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void URubezhInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	URubezhInventoryItemWidget* DragVisual = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer(), GetClass());
	DragVisual->Setup(nullptr, Source, EntryIndex, Entry, Definition);

	URubezhInventoryDragOperation* Operation = NewObject<URubezhInventoryDragOperation>(this);
	Operation->Source = Source;
	Operation->EntryIndex = EntryIndex;
	Operation->ItemId = Entry.ItemId;
	Operation->DefaultDragVisual = DragVisual;
	Operation->Pivot = EDragPivot::CenterCenter;
	OutOperation = Operation;

	SetRenderOpacity(Rubezh::InventoryStyle::DraggedItemOpacity);
}

void URubezhInventoryItemWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetRenderOpacity(1.0f);
	if (Window.IsValid())
	{
		Window->HideDropPreview();
	}
}
