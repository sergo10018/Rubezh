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
	WidgetTree->RootWidget = SizeBox;

	Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetPadding(FMargin(4.0f, 2.0f));
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

void URubezhInventoryItemWidget::Setup(URubezhInventoryWindow* InWindow, int32 InEntryIndex, const FRubezhInventoryEntry& InEntry, const FRubezhItemDefinition& InDefinition)
{
	using namespace Rubezh::InventoryStyle;

	Window = InWindow;
	EntryIndex = InEntryIndex;
	Entry = InEntry;
	Definition = InDefinition;

	const FIntPoint Footprint = GetFootprint();
	SizeBox->SetWidthOverride(Footprint.X * CellSize - CellGap);
	SizeBox->SetHeightOverride(Footprint.Y * CellSize - CellGap);
	Frame->SetBrushColor(GetCategoryColor(Definition.Category));

	NameText->SetText(Definition.DisplayName);
	CountText->SetText(Entry.Count > 1 ? FText::AsNumber(Entry.Count) : FText::GetEmpty());
	SetToolTipText(BuildTooltip());
	SetVisibility(InWindow ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

FText URubezhInventoryItemWidget::BuildTooltip() const
{
	const float StackWeight = Definition.WeightKg * Entry.Count;
	const FString Stats = Rubezh::Equipment::DescribeStats(Definition);
	FString UseHint;
	if (Rubezh::Injuries::IsMedical(Definition))
	{
		UseHint = TEXT("\nДвойной клик — применить");
	}
	else if (Definition.EquipSlot != ERubezhEquipSlot::None)
	{
		UseHint = FString::Printf(TEXT("\nДвойной клик или перетащить в слот «%s» — надеть"), Rubezh::Equipment::GetSlotName(Definition.EquipSlot));
	}
	return FText::FromString(FString::Printf(TEXT("%s\n%s%d шт. × %.3g кг = %.2f кг\nЛКМ — переложить, ПКМ — повернуть%s"),
		*Definition.DisplayName.ToString(), Stats.IsEmpty() ? TEXT("") : *(Stats + TEXT("\n")),
		Entry.Count, Definition.WeightKg, StackWeight, *UseHint));
}

FReply URubezhInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!Window.IsValid())
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Window->RequestRotate(EntryIndex);
		return FReply::Handled();
	}

	GrabOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply URubezhInventoryItemWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Window.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (Rubezh::Injuries::IsMedical(Definition))
		{
			Window->RequestUse(EntryIndex);
			return FReply::Handled();
		}
		if (Definition.EquipSlot != ERubezhEquipSlot::None)
		{
			Window->RequestEquip(EntryIndex);
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void URubezhInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	URubezhInventoryItemWidget* DragVisual = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer(), GetClass());
	DragVisual->Setup(nullptr, EntryIndex, Entry, Definition);

	URubezhInventoryDragOperation* Operation = NewObject<URubezhInventoryDragOperation>(this);
	Operation->EntryIndex = EntryIndex;
	Operation->bRotated = Entry.bRotated;
	Operation->Footprint = GetFootprint();
	Operation->GrabOffset = GrabOffset;
	Operation->DefaultDragVisual = DragVisual;
	Operation->Pivot = EDragPivot::MouseDown;
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
