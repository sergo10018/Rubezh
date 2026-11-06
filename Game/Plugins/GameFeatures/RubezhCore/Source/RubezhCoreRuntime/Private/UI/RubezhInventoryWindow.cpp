// РУБЕЖ: Земля после разлома

#include "UI/RubezhInventoryWindow.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
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
#include "Health/RubezhInjuryRules.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhLootContainer.h"
#include "Items/RubezhLootRules.h"
#include "RubezhSettings.h"
#include "Survival/RubezhSurvivalSet.h"
#include "TimerManager.h"
#include "UI/RubezhEquipmentSlotWidget.h"
#include "UI/RubezhInventoryStyle.h"
#include "UI/RubezhSlotGridWidget.h"
#include "UI/RubezhTraderPanel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryWindow)

namespace
{
	constexpr float WeightBarHeight = 8.0f;
	constexpr float SectionSpacing = 10.0f;
	constexpr float ButtonGap = 6.0f;

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
	Columns->AddChildToHorizontalBox(BuildBackpackColumn());
	Columns->AddChildToHorizontalBox(BuildLootColumn())->SetPadding(FMargin(ColumnGap, 0.0f, 0.0f, 0.0f));

	TraderPanel = CreateWidget<URubezhTraderPanel>(this);
	TraderPanel->SetVisibility(ESlateVisibility::Collapsed);
	Columns->AddChildToHorizontalBox(TraderPanel)->SetPadding(FMargin(ColumnGap, 0.0f, 0.0f, 0.0f));
}

UWidget* URubezhInventoryWindow::BuildEquipmentColumn()
{
	using namespace Rubezh::InventoryStyle;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->AddChildToVerticalBox(MakeText(*WidgetTree, FText::FromString(TEXT("Снаряжение")), TitleFontSize));
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	for (int32 Index = 0; Index < Rubezh::Weapons::SlotCount; ++Index)
	{
		URubezhEquipmentSlotWidget* SlotWidget = CreateWidget<URubezhEquipmentSlotWidget>(this);
		SlotWidget->SetupWeapon(this, Index);
		Column->AddChildToVerticalBox(SlotWidget)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, EquipSlotGap));
		WeaponSlotWidgets.Add(SlotWidget);
	}
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

UWidget* URubezhInventoryWindow::BuildBackpackColumn()
{
	using namespace Rubezh::InventoryStyle;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	TitleText = MakeText(*WidgetTree, FText::FromString(TEXT("Рюкзак")), TitleFontSize);
	Column->AddChildToVerticalBox(TitleText);

	// Подсказка не должна растягивать окно: переносится по ширине сетки.
	USizeBox* HintBox = WidgetTree->ConstructWidget<USizeBox>();
	HintBox->SetWidthOverride(SlotColumns * CellSize);
	HintLine = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize - 2, HintText);
	HintLine->SetAutoWrapText(true);
	HintBox->AddChild(HintLine);
	Column->AddChildToVerticalBox(HintBox);
	ShowGroundHint(false);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	MoneyText = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize);
	Column->AddChildToVerticalBox(MoneyText);

	WeightText = MakeText(*WidgetTree, FText::GetEmpty(), TextFontSize);
	Column->AddChildToVerticalBox(WeightText);

	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>();
	BarBox->SetHeightOverride(WeightBarHeight);
	WeightBar = WidgetTree->ConstructWidget<UProgressBar>();
	BarBox->AddChild(WeightBar);
	Column->AddChildToVerticalBox(BarBox);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	BackpackGrid = CreateWidget<URubezhSlotGridWidget>(this);
	BackpackGrid->Setup(this, ERubezhItemSource::Backpack);
	Column->AddChildToVerticalBox(BackpackGrid)->SetHorizontalAlignment(HAlign_Left);
	return Column;
}

UWidget* URubezhInventoryWindow::BuildLootColumn()
{
	using namespace Rubezh::InventoryStyle;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	LootTitle = MakeText(*WidgetTree, FText::GetEmpty(), TitleFontSize);
	Column->AddChildToVerticalBox(LootTitle);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* TakeAll = MakeButton(TEXT("Забрать всё"));
	TakeAll->OnClicked.AddDynamic(this, &ThisClass::HandleTakeAllClicked);
	Buttons->AddChildToHorizontalBox(TakeAll)->SetPadding(FMargin(0.0f, 0.0f, ButtonGap, 0.0f));
	WearBagButton = MakeButton(TEXT("Надеть этот рюкзак"));
	WearBagButton->OnClicked.AddDynamic(this, &ThisClass::HandleWearBagClicked);
	Buttons->AddChildToHorizontalBox(WearBagButton);
	Column->AddChildToVerticalBox(Buttons);
	AddSpacer(*WidgetTree, *Column, SectionSpacing);

	LootGrid = CreateWidget<URubezhSlotGridWidget>(this);
	LootGrid->Setup(this, ERubezhItemSource::Loot);
	Column->AddChildToVerticalBox(LootGrid)->SetHorizontalAlignment(HAlign_Left);

	LootColumn = Column;
	LootColumn->SetVisibility(ESlateVisibility::Collapsed);
	return Column;
}

UButton* URubezhInventoryWindow::MakeButton(const FString& Label)
{
	using namespace Rubezh::InventoryStyle;

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(ButtonColor);
	Button->SetContent(MakeText(*WidgetTree, FText::FromString(Label), TextFontSize));
	return Button;
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
		Inventory->ServerStopLoadingMagazine();
		if (Inventory->GetLootContainer())
		{
			Inventory->ServerCloseLoot();
		}
		if (Inventory->GetTraderInUse())
		{
			Inventory->ServerCloseTrader();
		}
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

bool URubezhInventoryWindow::IsLootOpen() const
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	return Inventory && Inventory->GetLootContainer();
}

void URubezhInventoryWindow::Refresh()
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		WeightText->SetText(FText::FromString(TEXT("Рюкзак не найден")));
		return;
	}

	BackpackGrid->SetContents(Inventory->GetGrid(), [Inventory](FName Id) { return Inventory->FindDefinition(Id); });
	MoneyText->SetText(FText::FromString(FString::Printf(TEXT("Билеты: %d"), Inventory->GetMoney())));
	RefreshWeight(*Inventory);
	RefreshEquipment(*Inventory);
	RefreshLoot(*Inventory);

	TraderPanel->SetVisibility(Inventory->GetTraderInUse() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TraderPanel->Refresh(*Inventory);
}

void URubezhInventoryWindow::RefreshLoot(const URubezhInventoryComponent& Inventory)
{
	const ARubezhLootContainer* Container = Inventory.GetLootContainer();
	LootColumn->SetVisibility(Container ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!Container)
	{
		return;
	}

	LootTitle->SetText(Container->GetDisplayName());
	WearBagButton->SetVisibility(Container->GetBagItemId().IsNone() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	LootGrid->SetContents(Inventory.GetLootGrid(), [&Inventory](FName Id) { return Inventory.FindDefinition(Id); });
}

void URubezhInventoryWindow::RefreshEquipment(const URubezhInventoryComponent& Inventory)
{
	for (URubezhEquipmentSlotWidget* SlotWidget : EquipSlots)
	{
		const FName ItemId = Inventory.GetEquipped(SlotWidget->GetEquipSlot());
		SlotWidget->SetItem(ItemId, ItemId.IsNone() ? nullptr : Inventory.FindDefinition(ItemId));
		SlotWidget->SetRenderOpacity(1.0f);
	}

	const Rubezh::Weapons::FWeaponSlots& Weapons = Inventory.GetWeaponSlots();
	for (URubezhEquipmentSlotWidget* SlotWidget : WeaponSlotWidgets)
	{
		const FRubezhInventoryEntry Entry = Weapons.IsValidIndex(SlotWidget->GetWeaponSlot()) ? Weapons[SlotWidget->GetWeaponSlot()] : FRubezhInventoryEntry();
		SlotWidget->SetWeapon(Entry, Entry.ItemId.IsNone() ? nullptr : Inventory.FindDefinition(Entry.ItemId));
		SlotWidget->SetRenderOpacity(1.0f);
	}

	const FName Backpack = Inventory.GetEquipped(ERubezhEquipSlot::Backpack);
	const FRubezhItemDefinition* BackpackItem = Backpack.IsNone() ? nullptr : Inventory.FindDefinition(Backpack);
	const FRubezhInventoryGrid& Grid = Inventory.GetGrid();
	TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s · %d/%d"),
		BackpackItem ? *BackpackItem->DisplayName.ToString() : TEXT("Карманы (рюкзака нет)"), Grid.GetEntries().Num(), Grid.GetSlotCount())));

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

const FRubezhItemDefinition* URubezhInventoryWindow::FindEntryDefinition(int32 EntryIndex) const
{
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	return (Inventory && Inventory->GetGrid().GetEntries().IsValidIndex(EntryIndex))
		? Inventory->FindDefinition(Inventory->GetGrid().GetEntries()[EntryIndex].ItemId) : nullptr;
}

void URubezhInventoryWindow::QuickMove(ERubezhItemSource Source, int32 EntryIndex)
{
	if (!IsLootOpen())
	{
		return;
	}
	if (Source == ERubezhItemSource::Backpack)
	{
		PutIntoLoot(EntryIndex, INDEX_NONE);
	}
	else if (Source == ERubezhItemSource::Loot)
	{
		TakeFromLoot(EntryIndex, INDEX_NONE);
	}
}

void URubezhInventoryWindow::Activate(ERubezhItemSource Source, int32 EntryIndex)
{
	if (Source == ERubezhItemSource::Loot)
	{
		TakeFromLoot(EntryIndex, INDEX_NONE);
		return;
	}

	const FRubezhItemDefinition* Item = FindEntryDefinition(EntryIndex);
	const URubezhInventoryComponent* Inventory = BoundInventory.Get();
	const FRubezhInventoryEntry* Entry = (Item && Inventory) ? &Inventory->GetGrid().GetEntries()[EntryIndex] : nullptr;
	if (Entry && Rubezh::Items::FindWeapon(Entry->ItemId))
	{
		RequestEquipWeapon(EntryIndex, {});
	}
	else if (Entry && Item->MagazineCapacity > 0)
	{
		BoundInventory->ServerUnloadMagazine(EntryIndex);
	}
	else if (Item && (Rubezh::Injuries::IsMedical(*Item) || Item->CleanPercent > 0.0f))
	{
		RequestUse(EntryIndex);
	}
	else if (Item && Item->EquipSlot != ERubezhEquipSlot::None)
	{
		RequestEquip(EntryIndex);
	}
}

void URubezhInventoryWindow::HandleGridDrop(ERubezhItemSource Target, int32 Cell, const URubezhInventoryDragOperation& Operation)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	const bool bToBackpack = Target == ERubezhItemSource::Backpack;
	switch (Operation.Source)
	{
	case ERubezhItemSource::Backpack:
		if (!bToBackpack)
		{
			PutIntoLoot(Operation.EntryIndex, Cell);
		}
		else if (Cell != INDEX_NONE && !TryLoadMagazine(Operation, Cell))
		{
			Inventory->ServerMoveEntry(Operation.EntryIndex, Cell);
		}
		break;

	case ERubezhItemSource::Loot:
		if (bToBackpack)
		{
			TakeFromLoot(Operation.EntryIndex, Cell);
		}
		else if (Cell != INDEX_NONE)
		{
			Inventory->ServerMoveLootEntry(Operation.EntryIndex, Operation.ItemId, Cell);
		}
		break;

	case ERubezhItemSource::Weapon:
		if (bToBackpack)
		{
			RequestUnequipWeapon(Operation.FromWeaponSlot, Cell == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(Cell));
			return;
		}
		break;

	case ERubezhItemSource::Equipment:
		if (bToBackpack)
		{
			RequestUnequip(Operation.FromSlot, Cell == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(Cell));
			return;
		}
		break;
	}
	// У клиента новое содержимое придёт с сервера позже; перерисовка сейчас возвращает плитке непрозрачность.
	Refresh();
}

void URubezhInventoryWindow::PutIntoLoot(int32 EntryIndex, int32 TargetSlot)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->GetLootContainer() || !Inventory->GetGrid().GetEntries().IsValidIndex(EntryIndex))
	{
		return;
	}

	// Проверяем у себя теми же правилами, что и сервер, — чтобы сразу сказать, почему нельзя.
	FRubezhInventoryGrid From = Inventory->GetGrid();
	FRubezhInventoryGrid To = Inventory->GetLootGrid();
	const TOptional<int32> Target = TargetSlot == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(TargetSlot);
	if (Rubezh::Loot::Transfer(From, EntryIndex, To, Target, [Inventory](FName Id) { return Inventory->FindDefinition(Id); }) == 0)
	{
		ShowError(TEXT("Туда не помещается — нет свободной ячейки"));
	}
	else
	{
		Inventory->ServerPutLoot(EntryIndex, Inventory->GetGrid().GetEntries()[EntryIndex].ItemId, TargetSlot);
	}
	Refresh();
}

void URubezhInventoryWindow::TakeFromLoot(int32 LootEntryIndex, int32 TargetSlot)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !Inventory->GetLootGrid().GetEntries().IsValidIndex(LootEntryIndex))
	{
		return;
	}

	FRubezhInventoryGrid From = Inventory->GetLootGrid();
	FRubezhInventoryGrid To = Inventory->GetGrid();
	const TOptional<int32> Target = TargetSlot == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(TargetSlot);
	if (Rubezh::Loot::Transfer(From, LootEntryIndex, To, Target, [Inventory](FName Id) { return Inventory->FindDefinition(Id); }) == 0)
	{
		ShowError(TEXT("В рюкзаке нет свободной ячейки"));
	}
	else
	{
		Inventory->ServerTakeLoot(LootEntryIndex, Inventory->GetLootGrid().GetEntries()[LootEntryIndex].ItemId, TargetSlot);
	}
	Refresh();
}

void URubezhInventoryWindow::HandleTakeAllClicked()
{
	if (URubezhInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->ServerTakeAllLoot();
	}
}

void URubezhInventoryWindow::HandleWearBagClicked()
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	const ARubezhLootContainer* Container = Inventory ? Inventory->GetLootContainer() : nullptr;
	if (!Container)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::WearBag(Grid, Equipped, Container->GetBagItemId(),
		Inventory->GetLootGrid(), [Inventory](FName Id) { return Inventory->FindDefinition(Id); });
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		ShowError(Rubezh::Equipment::Describe(Result));
		return;
	}
	Inventory->ServerWearLootBag();
}

void URubezhInventoryWindow::RequestEquip(int32 EntryIndex)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Equip(Grid, Equipped, EntryIndex,
		[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketSlots());
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		ShowError(Rubezh::Equipment::Describe(Result));
	}
	else
	{
		Inventory->ServerEquipEntry(EntryIndex);
	}
	Refresh();
}

void URubezhInventoryWindow::RequestUnequip(ERubezhEquipSlot EquipSlot, TOptional<int32> TargetSlot)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, EquipSlot, TargetSlot,
		[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketSlots());
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		ShowError(Rubezh::Equipment::Describe(Result));
	}
	else
	{
		Inventory->ServerUnequip(EquipSlot, TargetSlot.Get(INDEX_NONE));
	}
	Refresh();
}

void URubezhInventoryWindow::RequestEquipWeapon(int32 EntryIndex, TOptional<int32> WeaponSlot)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Weapons::FWeaponSlots Slots = Inventory->GetWeaponSlots();
	const Rubezh::Weapons::EResult Result = Rubezh::Weapons::Equip(Grid, Slots, EntryIndex, WeaponSlot, [](FName Id) { return Rubezh::Items::FindWeapon(Id); });
	if (Result != Rubezh::Weapons::EResult::Ok)
	{
		ShowError(Rubezh::Weapons::Describe(Result));
	}
	else
	{
		Inventory->ServerEquipWeapon(EntryIndex, WeaponSlot.Get(INDEX_NONE));
	}
	Refresh();
}

void URubezhInventoryWindow::RequestUnequipWeapon(int32 WeaponSlot, TOptional<int32> Cell)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	FRubezhInventoryGrid Grid = Inventory->GetGrid();
	Rubezh::Weapons::FWeaponSlots Slots = Inventory->GetWeaponSlots();
	const Rubezh::Weapons::EResult Result = Rubezh::Weapons::Unequip(Grid, Slots, WeaponSlot, Cell);
	if (Result != Rubezh::Weapons::EResult::Ok)
	{
		ShowError(Rubezh::Weapons::Describe(Result));
	}
	else
	{
		Inventory->ServerUnequipWeapon(WeaponSlot, Cell.Get(INDEX_NONE));
	}
	Refresh();
}

bool URubezhInventoryWindow::TryLoadMagazine(const URubezhInventoryDragOperation& Operation, int32 Cell)
{
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return false;
	}

	const int32 MagazineIndex = Inventory->GetGrid().FindEntryAt(Cell);
	const FRubezhItemDefinition* Magazine = MagazineIndex == INDEX_NONE ? nullptr : FindEntryDefinition(MagazineIndex);
	if (!Magazine || Magazine->MagazineCapacity <= 0 || Magazine->AmmoItem != Operation.ItemId)
	{
		return false;
	}

	Inventory->ServerStartLoadingMagazine(MagazineIndex);
	return true;
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
	ShowGroundHint(false);
}

bool URubezhInventoryWindow::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const URubezhInventoryDragOperation* Operation = Cast<URubezhInventoryDragOperation>(InOperation);
	if (!Operation)
	{
		return false;
	}

	// Сюда доходит, только когда курсор не над ячейками и не над слотом снаряжения.
	BackpackGrid->HideHighlight();
	LootGrid->HideHighlight();
	ShowGroundHint(Operation->Source != ERubezhItemSource::Loot && !IsOverWindow(InDragDropEvent));
	return true;
}

bool URubezhInventoryWindow::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideDropPreview();

	const URubezhInventoryDragOperation* Operation = Cast<URubezhInventoryDragOperation>(InOperation);
	URubezhInventoryComponent* Inventory = BoundInventory.Get();
	if (!Operation || !Inventory)
	{
		return false;
	}

	// За окном — бросить на землю (из обыскиваемого — нельзя: сначала забрать себе).
	if (!IsOverWindow(InDragDropEvent))
	{
		if (Operation->Source == ERubezhItemSource::Equipment)
		{
			Inventory->ServerDropEquipped(Operation->FromSlot);
		}
		else if (Operation->Source == ERubezhItemSource::Weapon)
		{
			Inventory->ServerDropWeapon(Operation->FromWeaponSlot);
		}
		else if (Operation->Source == ERubezhItemSource::Backpack)
		{
			Inventory->ServerDropEntry(Operation->EntryIndex);
		}
	}
	// В окне, но мимо ячеек — ничего не делаем; перерисовка вернёт плитке непрозрачность.
	Refresh();
	return true;
}

void URubezhInventoryWindow::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HideDropPreview();
}

void URubezhInventoryWindow::HideDropPreview()
{
	if (BackpackGrid)
	{
		BackpackGrid->HideHighlight();
	}
	if (LootGrid)
	{
		LootGrid->HideHighlight();
	}
	ShowGroundHint(false);
}

bool URubezhInventoryWindow::IsOverWindow(const FDragDropEvent& DragDropEvent) const
{
	return WindowPanel->GetCachedGeometry().IsUnderLocation(DragDropEvent.GetScreenSpacePosition());
}

void URubezhInventoryWindow::ShowGroundHint(bool bShow)
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
		TEXT("ЛКМ — переложить · патроны на магазин — набить · ПКМ — в ящик и обратно · двойной клик — применить, надеть, забрать, разрядить · за окно — выбросить · %s — закрыть"), *CloseKey)));
	HintLine->SetColorAndOpacity(FSlateColor(HintText));
}
