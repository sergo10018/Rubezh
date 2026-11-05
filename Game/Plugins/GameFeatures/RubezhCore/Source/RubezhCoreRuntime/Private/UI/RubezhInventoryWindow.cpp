// РУБЕЖ: Земля после разлома

#include "UI/RubezhInventoryWindow.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Health/RubezhInjuryComponent.h"
#include "Input/UIActionBindingHandle.h"
#include "Items/RubezhInventoryComponent.h"
#include "RubezhSettings.h"
#include "Survival/RubezhSurvivalSet.h"
#include "UI/RubezhInventoryItemWidget.h"
#include "UI/RubezhInventoryStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryWindow)

namespace
{
	constexpr int32 CellZOrder = 0;
	constexpr int32 ItemZOrder = 1;
	constexpr int32 PreviewZOrder = 2;
	constexpr float WeightBarHeight = 8.0f;
	constexpr float SectionSpacing = 10.0f;

	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Text, int32 FontSize, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Block = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetText(Text);
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	void AddSpacer(UWidgetTree& Tree, UVerticalBox& Box, float Height)
	{
		USpacer* Spacer = Tree.ConstructWidget<USpacer>();
		Spacer->SetSize(FVector2D(0.0f, Height));
		Box.AddChildToVerticalBox(Spacer);
	}
}

URubezhInventoryWindow::URubezhInventoryWindow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
}

void URubezhInventoryWindow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void URubezhInventoryWindow::BuildLayout()
{
	using namespace Rubezh::InventoryStyle;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
	Dim->SetBrushColor(ScreenDim);
	UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
	DimSlot->SetHorizontalAlignment(HAlign_Fill);
	DimSlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(WindowBackground);
	Panel->SetPadding(FMargin(WindowPadding));
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);

	Column->AddChildToVerticalBox(MakeText(*WidgetTree, FText::FromString(TEXT("Рюкзак")), TitleFontSize));
	HintLine = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize - 2, HintText);
	Column->AddChildToVerticalBox(HintLine);
	ShowDropToGroundHint(false);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	WeightText = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize);
	Column->AddChildToVerticalBox(WeightText);

	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>();
	BarBox->SetHeightOverride(WeightBarHeight);
	WeightBar = WidgetTree->ConstructWidget<UProgressBar>();
	BarBox->AddChild(WeightBar);
	Column->AddChildToVerticalBox(BarBox);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	UBorder* GridFrame = WidgetTree->ConstructWidget<UBorder>();
	GridFrame->SetBrushColor(GridBackground);
	GridFrame->SetPadding(FMargin(CellGap));
	UVerticalBoxSlot* GridSlot = Column->AddChildToVerticalBox(GridFrame);
	GridSlot->SetHorizontalAlignment(HAlign_Center);

	GridSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	GridFrame->SetContent(GridSizeBox);
	GridCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	GridSizeBox->AddChild(GridCanvas);

	DropPreview = WidgetTree->ConstructWidget<UBorder>();
	DropPreview->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* PreviewSlot = GridCanvas->AddChildToCanvas(DropPreview);
	PreviewSlot->SetZOrder(PreviewZOrder);
}

void URubezhInventoryWindow::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (URubezhInventoryComponent* Inventory = FindInventory())
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::Refresh);
	}
	Refresh();
}

void URubezhInventoryWindow::NativeOnDeactivated()
{
	if (URubezhInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::Refresh);
	}
	BoundInventory.Reset();

	Super::NativeOnDeactivated();
}

TOptional<FUIInputConfig> URubezhInventoryWindow::GetDesiredInputConfig() const
{
	// Как у меню: курсор виден, клавиши не доходят до персонажа.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* URubezhInventoryWindow::NativeGetDesiredFocusTarget() const
{
	// Фокус на самом окне — иначе оно не получит нажатие клавиши закрытия.
	return const_cast<URubezhInventoryWindow*>(this);
}

FReply URubezhInventoryWindow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == GetDefault<URubezhSettings>()->InventoryKey || Key == EKeys::Escape)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

URubezhInventoryComponent* URubezhInventoryWindow::FindInventory() const
{
	return URubezhInventoryComponent::FindForActor(GetOwningPlayer());
}

void URubezhInventoryWindow::Refresh()
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		WeightText->SetText(FText::FromString(TEXT("Рюкзак не найден")));
		return;
	}

	RebuildCells(Inventory->GetGrid().GetSize());
	RefreshWeight(*Inventory);
	RefreshItems(*Inventory);
}

void URubezhInventoryWindow::RebuildCells(FIntPoint GridSize)
{
	using namespace Rubezh::InventoryStyle;

	if (GridSize == BuiltGridSize)
	{
		return;
	}
	BuiltGridSize = GridSize;

	for (UWidget* Cell : CellWidgets)
	{
		Cell->RemoveFromParent();
	}
	CellWidgets.Reset();

	GridSizeBox->SetWidthOverride(GridSize.X * CellSize - CellGap);
	GridSizeBox->SetHeightOverride(GridSize.Y * CellSize - CellGap);

	for (int32 Y = 0; Y < GridSize.Y; ++Y)
	{
		for (int32 X = 0; X < GridSize.X; ++X)
		{
			UBorder* Cell = WidgetTree->ConstructWidget<UBorder>();
			Cell->SetBrushColor(EmptyCell);
			Cell->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* CellSlot = GridCanvas->AddChildToCanvas(Cell);
			CellSlot->SetPosition(FVector2D(X * CellSize, Y * CellSize));
			CellSlot->SetSize(FVector2D(CellSize - CellGap, CellSize - CellGap));
			CellSlot->SetZOrder(CellZOrder);
			CellWidgets.Add(Cell);
		}
	}
}

void URubezhInventoryWindow::RefreshWeight(const URubezhInventoryComponent& Inventory)
{
	using namespace Rubezh::InventoryStyle;

	float MaxWeight = Rubezh::Encumbrance::BaseMaxCarryWeight;
	if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Inventory.GetOwner()))
	{
		bool bFound = false;
		const float Value = ASC->GetGameplayAttributeValue(URubezhSurvivalSet::GetMaxCarryWeightAttribute(), bFound);
		MaxWeight = bFound ? Value : MaxWeight;
	}

	const float Weight = Inventory.GetTotalWeight();
	const ERubezhEncumbranceLevel Level = Rubezh::Encumbrance::GetLevel(Weight, MaxWeight);
	const FLinearColor LevelColor = GetEncumbranceColor(Level);

	WeightText->SetText(FText::FromString(FString::Printf(TEXT("Вес: %.1f / %.0f кг — %s"), Weight, MaxWeight, GetEncumbranceName(Level))));
	WeightText->SetColorAndOpacity(FSlateColor(LevelColor));
	WeightBar->SetPercent(MaxWeight > 0.0f ? FMath::Clamp(Weight / MaxWeight, 0.0f, 1.0f) : 1.0f);
	WeightBar->SetFillColorAndOpacity(LevelColor);
}

void URubezhInventoryWindow::RefreshItems(const URubezhInventoryComponent& Inventory)
{
	using namespace Rubezh::InventoryStyle;

	for (UWidget* Item : ItemWidgets)
	{
		Item->RemoveFromParent();
	}
	ItemWidgets.Reset();
	HideDropPreview();

	const TArray<FRubezhInventoryEntry>& Entries = Inventory.GetGrid().GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FRubezhInventoryEntry& Entry = Entries[Index];
		const FRubezhItemDefinition* Definition = Inventory.FindDefinition(Entry.ItemId);
		if (!Definition)
		{
			continue;
		}

		URubezhInventoryItemWidget* Item = CreateWidget<URubezhInventoryItemWidget>(GetOwningPlayer());
		Item->Setup(this, Index, Entry, *Definition);
		UCanvasPanelSlot* ItemSlot = GridCanvas->AddChildToCanvas(Item);
		ItemSlot->SetPosition(FVector2D(Entry.Position.X * CellSize, Entry.Position.Y * CellSize));
		ItemSlot->SetAutoSize(true);
		ItemSlot->SetZOrder(ItemZOrder);
		ItemWidgets.Add(Item);
	}
}

void URubezhInventoryWindow::RequestRotate(int32 EntryIndex)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->GetGrid().GetEntries().IsValidIndex(EntryIndex))
	{
		return;
	}

	// Поворот на месте; если повёрнутый предмет не помещается, сервер откажет и ничего не изменится.
	const FRubezhInventoryEntry& Entry = Inventory->GetGrid().GetEntries()[EntryIndex];
	Inventory->ServerMoveEntry(EntryIndex, Entry.Position, !Entry.bRotated);
}

void URubezhInventoryWindow::RequestUse(int32 EntryIndex)
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	URubezhInjuryComponent* Injuries = URubezhInjuryComponent::FindForActor(GetOwningPlayerPawn());
	if (Inventory && Injuries && Inventory->GetGrid().GetEntries().IsValidIndex(EntryIndex))
	{
		Injuries->UseItem(Inventory->GetGrid().GetEntries()[EntryIndex].ItemId);
	}
}

bool URubezhInventoryWindow::EvaluateDrop(const FDragDropEvent& DragDropEvent, const URubezhInventoryDragOperation& Operation, FIntPoint& OutCell) const
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return false;
	}

	using Rubezh::InventoryStyle::CellSize;
	const FVector2D TopLeft = GridCanvas->GetCachedGeometry().AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition()) - Operation.GrabOffset;
	OutCell = FIntPoint(FMath::RoundToInt(TopLeft.X / CellSize), FMath::RoundToInt(TopLeft.Y / CellSize));

	auto Lookup = [Inventory](FName ItemId) { return Inventory->FindDefinition(ItemId); };
	return Inventory->GetGrid().IsAreaFree(OutCell, Operation.Footprint, Lookup, Operation.EntryIndex);
}

bool URubezhInventoryWindow::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const URubezhInventoryDragOperation* Operation = Cast<URubezhInventoryDragOperation>(InOperation);
	if (!Operation)
	{
		return false;
	}

	using namespace Rubezh::InventoryStyle;
	if (!IsOverGrid(InDragDropEvent))
	{
		HideDropPreview();
		ShowDropToGroundHint(true);
		return true;
	}
	ShowDropToGroundHint(false);

	FIntPoint Cell;
	const bool bFree = EvaluateDrop(InDragDropEvent, *Operation, Cell);

	const FIntPoint Size = BuiltGridSize;
	const bool bInsideGrid = Cell.X < Size.X && Cell.Y < Size.Y && Cell.X + Operation->Footprint.X > 0 && Cell.Y + Operation->Footprint.Y > 0;
	if (!bInsideGrid)
	{
		HideDropPreview();
		return true;
	}

	// Подсветка обрезается по краю сетки, чтобы не вылезать за окно.
	const FIntPoint ClampedMin(FMath::Max(Cell.X, 0), FMath::Max(Cell.Y, 0));
	const FIntPoint ClampedMax(FMath::Min(Cell.X + Operation->Footprint.X, Size.X), FMath::Min(Cell.Y + Operation->Footprint.Y, Size.Y));
	UCanvasPanelSlot* PreviewSlot = Cast<UCanvasPanelSlot>(DropPreview->Slot);
	PreviewSlot->SetPosition(FVector2D(ClampedMin.X * CellSize, ClampedMin.Y * CellSize));
	PreviewSlot->SetSize(FVector2D((ClampedMax.X - ClampedMin.X) * CellSize - CellGap, (ClampedMax.Y - ClampedMin.Y) * CellSize - CellGap));
	DropPreview->SetBrushColor(bFree ? DropAllowed : DropBlocked);
	DropPreview->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

bool URubezhInventoryWindow::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideDropPreview();

	const URubezhInventoryDragOperation* Operation = Cast<URubezhInventoryDragOperation>(InOperation);
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (Operation && Inventory && !IsOverGrid(InDragDropEvent))
	{
		Inventory->ServerDropEntry(Operation->EntryIndex);
		Refresh();
		return true;
	}

	FIntPoint Cell;
	if (!Operation || !Inventory || !EvaluateDrop(InDragDropEvent, *Operation, Cell))
	{
		// Не обработали — перетаскивание считается отменённым, плитка вернётся на место.
		return false;
	}

	Inventory->ServerMoveEntry(Operation->EntryIndex, Cell, Operation->bRotated);
	// У клиента новое содержимое придёт с сервера позже; перерисовка сейчас возвращает плитке непрозрачность.
	Refresh();
	return true;
}

void URubezhInventoryWindow::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideDropPreview();
}

void URubezhInventoryWindow::HideDropPreview()
{
	if (DropPreview)
	{
		DropPreview->SetVisibility(ESlateVisibility::Collapsed);
	}
	ShowDropToGroundHint(false);
}

bool URubezhInventoryWindow::IsOverGrid(const FDragDropEvent& DragDropEvent) const
{
	return GridCanvas->GetCachedGeometry().IsUnderLocation(DragDropEvent.GetScreenSpacePosition());
}

void URubezhInventoryWindow::ShowDropToGroundHint(bool bShow)
{
	using namespace Rubezh::InventoryStyle;
	if (!HintLine)
	{
		return;
	}

	if (bShow)
	{
		HintLine->SetText(FText::FromString(TEXT("Отпустите — выбросить на землю")));
		HintLine->SetColorAndOpacity(FSlateColor(WarningText));
		return;
	}

	const FString CloseKey = GetDefault<URubezhSettings>()->InventoryKey.GetDisplayName().ToString();
	HintLine->SetText(FText::FromString(FString::Printf(
		TEXT("ЛКМ — переложить · ПКМ — повернуть · двойной клик — применить · за сетку — выбросить · %s — закрыть"), *CloseKey)));
	HintLine->SetColorAndOpacity(FSlateColor(HintText));
}
