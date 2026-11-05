// РУБЕЖ: Земля после разлома

#include "UI/RubezhInventoryWindow.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryComponent.h"
#include "TimerManager.h"
#include "UI/RubezhEquipmentSlotWidget.h"
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

	WindowPanel = Panel;

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->SetContent(Columns);
	Columns->AddChildToHorizontalBox(BuildEquipmentColumn())->SetPadding(FMargin(0.0f, 0.0f, ColumnGap, 0.0f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Columns->AddChildToHorizontalBox(Column);

	TitleText = MakeText(*WidgetTree, FText::FromString(TEXT("Рюкзак")), TitleFontSize);
	Column->AddChildToVerticalBox(TitleText);
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

UWidget* URubezhInventoryWindow::BuildEquipmentColumn()
{
	using namespace Rubezh::InventoryStyle;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->AddChildToVerticalBox(MakeText(*WidgetTree, FText::FromString(TEXT("Снаряжение")), TitleFontSize));
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	for (const ERubezhEquipSlot EquipSlot : Rubezh::Equipment::AllSlots)
	{
		URubezhEquipmentSlotWidget* SlotWidget = CreateWidget<URubezhEquipmentSlotWidget>(this);
		SlotWidget->Setup(this, EquipSlot);
		Column->AddChildToVerticalBox(SlotWidget)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, EquipSlotGap));
		EquipSlots.Add(SlotWidget);
	}

	AddSpacer(*WidgetTree, *Column, SectionSpacing);
	ProtectionText = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize - 1, HintText);
	ProtectionText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(ProtectionText);
	return Column;
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
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ErrorTimer);
	}
	bShowingError = false;
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
	RefreshEquipment(*Inventory);
}

void URubezhInventoryWindow::RefreshEquipment(const URubezhInventoryComponent& Inventory)
{
	for (URubezhEquipmentSlotWidget* SlotWidget : EquipSlots)
	{
		const FName ItemId = Inventory.GetEquipped(SlotWidget->GetEquipSlot());
		SlotWidget->SetItem(ItemId, ItemId.IsNone() ? nullptr : Inventory.FindDefinition(ItemId));
		SlotWidget->SetRenderOpacity(1.0f);
	}

	const FName Backpack = Inventory.GetEquipped(ERubezhEquipSlot::Backpack);
	const FRubezhItemDefinition* BackpackItem = Backpack.IsNone() ? nullptr : Inventory.FindDefinition(Backpack);
	TitleText->SetText(FText::FromString(BackpackItem
		? FString::Printf(TEXT("Рюкзак: %s"), *BackpackItem->DisplayName.ToString())
		: FString(TEXT("Карманы (рюкзака нет)"))));

	// Сколько урона снимает снаряжение по частям тела.
	TArray<FString> Zones;
	for (const ERubezhBodyZone Zone : { ERubezhBodyZone::Head, ERubezhBodyZone::Torso, ERubezhBodyZone::Arms, ERubezhBodyZone::Legs })
	{
		const float Reduction = (1.0f - Inventory.GetProtection(Zone).DamageMultiplier) * 100.0f;
		Zones.Add(FString::Printf(TEXT("%s %.0f %%"), Rubezh::Equipment::GetZoneName(Zone), Reduction));
	}
	auto Lookup = [&Inventory](FName Id) { return Inventory.FindDefinition(Id); };
	ProtectionText->SetText(FText::FromString(FString::Printf(TEXT("Защита от урона:\n%s\nТепло: %.0f\nЛимит веса: +%.0f кг от рюкзака"),
		*FString::Join(Zones, TEXT("\n")), Rubezh::Equipment::GetWarmth(Inventory.GetEquippedItems(), Lookup),
		Rubezh::Equipment::GetCarryBonusKg(Inventory.GetEquippedItems(), Lookup))));
}

const FRubezhItemDefinition* URubezhInventoryWindow::FindEntryDefinition(int32 EntryIndex) const
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	return (Inventory && Inventory->GetGrid().GetEntries().IsValidIndex(EntryIndex))
		? Inventory->FindDefinition(Inventory->GetGrid().GetEntries()[EntryIndex].ItemId) : nullptr;
}

void URubezhInventoryWindow::RequestEquip(int32 EntryIndex)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	// Проверяем у себя теми же правилами, что и сервер, — чтобы сразу сказать, почему нельзя.
	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Equip(Grid, Equipped, EntryIndex,
		[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketsSize());
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		ShowError(Rubezh::Equipment::Describe(Result));
		Refresh();
		return;
	}
	Inventory->ServerEquipEntry(EntryIndex);
	Refresh();
}

void URubezhInventoryWindow::RequestUnequip(ERubezhEquipSlot EquipSlot, TOptional<FIntPoint> Cell, bool bRotated)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, EquipSlot, Cell, bRotated,
		[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketsSize());
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		ShowError(Rubezh::Equipment::Describe(Result));
		Refresh();
		return;
	}
	Inventory->ServerUnequip(EquipSlot, Cell.Get(FIntPoint::ZeroValue), bRotated, !Cell.IsSet());
	Refresh();
}

void URubezhInventoryWindow::ShowError(const FString& Message)
{
	using namespace Rubezh::InventoryStyle;
	bShowingError = true;
	HintLine->SetText(FText::FromString(Message));
	HintLine->SetColorAndOpacity(FSlateColor(WarningText));
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ErrorTimer, this, &ThisClass::ClearError, ErrorShowSeconds, false);
	}
}

void URubezhInventoryWindow::ClearError()
{
	bShowingError = false;
	ShowDropToGroundHint(false);
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
		ShowDropToGroundHint(!IsOverWindow(InDragDropEvent));
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
	const bool bFromSlot = Operation && Operation->FromSlot != ERubezhEquipSlot::None;

	// За окном — бросить на землю.
	if (Operation && Inventory && !IsOverWindow(InDragDropEvent))
	{
		if (bFromSlot)
		{
			Inventory->ServerDropEquipped(Operation->FromSlot);
		}
		else
		{
			Inventory->ServerDropEntry(Operation->EntryIndex);
		}
		Refresh();
		return true;
	}

	// В окне, но не над сеткой (например, над колонкой снаряжения) — ничего не делаем.
	if (Operation && !IsOverGrid(InDragDropEvent))
	{
		Refresh();
		return true;
	}

	FIntPoint Cell;
	if (bFromSlot && Inventory)
	{
		EvaluateDrop(InDragDropEvent, *Operation, Cell);
		RequestUnequip(Operation->FromSlot, Cell, Operation->bRotated);
		return true;
	}
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

bool URubezhInventoryWindow::IsOverWindow(const FDragDropEvent& DragDropEvent) const
{
	return WindowPanel->GetCachedGeometry().IsUnderLocation(DragDropEvent.GetScreenSpacePosition());
}

void URubezhInventoryWindow::ShowDropToGroundHint(bool bShow)
{
	using namespace Rubezh::InventoryStyle;
	if (!HintLine || (bShowingError && !bShow))
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
		TEXT("ЛКМ — переложить · ПКМ — повернуть · двойной клик — применить или надеть · за окно — выбросить · %s — закрыть"), *CloseKey)));
	HintLine->SetColorAndOpacity(FSlateColor(HintText));
}
