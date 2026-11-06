// РУБЕЖ: Земля после разлома

#include "UI/RubezhTraderPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhTradeRules.h"
#include "Settlement/RubezhTrader.h"
#include "TimerManager.h"
#include "UI/RubezhInventoryStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhTraderPanel)

namespace
{
	constexpr float TraderPanelWidth = 470.0f;
	constexpr float TraderRowsHeight = 560.0f;
	constexpr float TraderRowGap = 3.0f;
	constexpr float TraderSectionGap = 10.0f;
	constexpr float TraderButtonGap = 4.0f;

	UTextBlock* MakeTraderText(UWidgetTree& Tree, const FString& Text, int32 FontSize, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Block = Tree.ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetText(FText::FromString(Text));
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	FString DescribeWorn(const FRubezhItemDefinition& Item, const FRubezhInventoryEntry& Entry)
	{
		return FString::Printf(TEXT("%s (%.0f %%)"), *Item.DisplayName.ToString(), Entry.Durability);
	}
}

void URubezhTraderRow::Setup(const FString& Label, const FString& PrimaryLabel, TFunction<void()> InPrimary, const FString& SecondaryLabel, TFunction<void()> InSecondary)
{
	using namespace Rubezh::InventoryStyle;

	PrimaryAction = MoveTemp(InPrimary);
	SecondaryAction = MoveTemp(InSecondary);

	UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = Box;

	UTextBlock* LabelText = MakeTraderText(*WidgetTree, Label, TextFontSize - 1);
	LabelText->SetAutoWrapText(true);
	UHorizontalBoxSlot* LabelSlot = Box->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	UButton* Primary = WidgetTree->ConstructWidget<UButton>();
	Primary->SetBackgroundColor(ButtonColor);
	Primary->SetContent(MakeTraderText(*WidgetTree, PrimaryLabel, TextFontSize - 1));
	Primary->OnClicked.AddDynamic(this, &ThisClass::HandlePrimaryClicked);
	Box->AddChildToHorizontalBox(Primary)->SetPadding(FMargin(TraderButtonGap, 0.0f, 0.0f, 0.0f));

	if (SecondaryAction)
	{
		UButton* Secondary = WidgetTree->ConstructWidget<UButton>();
		Secondary->SetBackgroundColor(ButtonColor);
		Secondary->SetContent(MakeTraderText(*WidgetTree, SecondaryLabel, TextFontSize - 1));
		Secondary->OnClicked.AddDynamic(this, &ThisClass::HandleSecondaryClicked);
		Box->AddChildToHorizontalBox(Secondary)->SetPadding(FMargin(TraderButtonGap, 0.0f, 0.0f, 0.0f));
	}
}

void URubezhTraderRow::HandlePrimaryClicked()
{
	if (PrimaryAction)
	{
		PrimaryAction();
	}
}

void URubezhTraderRow::HandleSecondaryClicked()
{
	if (SecondaryAction)
	{
		SecondaryAction();
	}
}

void URubezhTraderPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace Rubezh::InventoryStyle;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(TraderPanelWidth);
	WidgetTree->RootWidget = Size;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Size->AddChild(Column);

	TitleText = MakeTraderText(*WidgetTree, FString(), TitleFontSize);
	Column->AddChildToVerticalBox(TitleText);

	ErrorText = MakeTraderText(*WidgetTree, FString(), TextFontSize - 1, WarningText);
	ErrorText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(ErrorText);

	USizeBox* RowsBox = WidgetTree->ConstructWidget<USizeBox>();
	RowsBox->SetHeightOverride(TraderRowsHeight);
	Rows = WidgetTree->ConstructWidget<UScrollBox>();
	RowsBox->AddChild(Rows);
	Column->AddChildToVerticalBox(RowsBox)->SetPadding(FMargin(0.0f, TraderSectionGap, 0.0f, 0.0f));
}

void URubezhTraderPanel::Refresh(URubezhInventoryComponent& Inventory)
{
	const ARubezhTrader* Trader = Inventory.GetTraderInUse();
	if (!Trader)
	{
		return;
	}

	const ERubezhTraderKind Kind = Trader->GetKind();
	TitleText->SetText(Trader->GetDisplayName());

	// Перестроить строки целиком проще всего: окно обновляется только при сделке, а не каждый кадр.
	Rows->ClearChildren();
	AddBuyRows(Inventory, Kind);
	AddSellRows(Inventory, Kind);
	if (Kind == ERubezhTraderKind::Gunsmith || Kind == ERubezhTraderKind::Armorer)
	{
		AddRepairRows(Inventory, Kind);
	}
}

void URubezhTraderPanel::AddHeader(const FString& Text)
{
	using namespace Rubezh::InventoryStyle;
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>();
	Spacer->SetSize(FVector2D(0.0f, TraderSectionGap));
	Rows->AddChild(Spacer);
	Rows->AddChild(MakeTraderText(*WidgetTree, Text, TextFontSize + 1, HintText));
}

void URubezhTraderPanel::AddRow(const FString& Label, const FString& PrimaryLabel, TFunction<void()> Primary, const FString& SecondaryLabel, TFunction<void()> Secondary)
{
	URubezhTraderRow* Row = CreateWidget<URubezhTraderRow>(this);
	Row->Setup(Label, PrimaryLabel, MoveTemp(Primary), SecondaryLabel, MoveTemp(Secondary));
	Row->SetPadding(FMargin(0.0f, 0.0f, 0.0f, TraderRowGap));
	Rows->AddChild(Row);
}

void URubezhTraderPanel::AddBuyRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind)
{
	bool bHasHeader = false;
	for (const FName ItemId : Rubezh::Items::GetItemIds())
	{
		const FRubezhItemDefinition* Item = Inventory.FindDefinition(ItemId);
		if (!Item || !Rubezh::Trade::IsSoldBy(*Item, Kind))
		{
			continue;
		}
		if (!bHasHeader)
		{
			AddHeader(TEXT("Продаёт"));
			bHasHeader = true;
		}

		const FString Label = FString::Printf(TEXT("%s — %s"), *Item->DisplayName.ToString(), *Rubezh::Trade::FormatMoney(Item->Price));
		URubezhInventoryComponent* InventoryPtr = &Inventory;
		const int32 StackSize = Item->MaxStack;
		if (StackSize > 1)
		{
			AddRow(Label, TEXT("Купить"), [this, InventoryPtr, ItemId, Kind] { RequestBuy(*InventoryPtr, ItemId, 1, Kind); },
				FString::Printf(TEXT("×%d"), StackSize), [this, InventoryPtr, ItemId, StackSize, Kind] { RequestBuy(*InventoryPtr, ItemId, StackSize, Kind); });
		}
		else
		{
			AddRow(Label, TEXT("Купить"), [this, InventoryPtr, ItemId, Kind] { RequestBuy(*InventoryPtr, ItemId, 1, Kind); });
		}
	}
}

void URubezhTraderPanel::AddSellRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind)
{
	const TArray<FRubezhInventoryEntry>& Entries = Inventory.GetGrid().GetEntries();
	auto Items = [&Inventory](FName Id) { return Inventory.FindDefinition(Id); };
	auto Weapons = [](FName Id) { return Rubezh::Items::FindWeapon(Id); };

	bool bHasHeader = false;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FRubezhInventoryEntry& Entry = Entries[Index];
		const FRubezhItemDefinition* Item = Inventory.FindDefinition(Entry.ItemId);
		if (!Item || !Rubezh::Trade::IsBoughtBy(*Item, Kind))
		{
			continue;
		}
		if (!bHasHeader)
		{
			AddHeader(TEXT("Купит из рюкзака"));
			bHasHeader = true;
		}

		const FString Name = (Rubezh::Items::FindWeapon(Entry.ItemId) || Rubezh::Equipment::CanWear(*Item)) ? DescribeWorn(*Item, Entry)
			: Entry.Count > 1 ? FString::Printf(TEXT("%s ×%d"), *Item->DisplayName.ToString(), Entry.Count)
			: Item->DisplayName.ToString();
		const FString Label = FString::Printf(TEXT("%s — %s"), *Name, *Rubezh::Trade::FormatMoney(Rubezh::Trade::GetSellValue(Entry, *Item, Items, Weapons)));
		URubezhInventoryComponent* InventoryPtr = &Inventory;
		const FName ItemId = Entry.ItemId;
		const int32 Cell = Entry.Slot;
		AddRow(Label, TEXT("Продать"), [InventoryPtr, Cell, ItemId] { InventoryPtr->ServerSellEntry(Cell, ItemId); });
	}
}

void URubezhTraderPanel::AddRepairRows(URubezhInventoryComponent& Inventory, ERubezhTraderKind Kind)
{
	bool bHasHeader = false;
	auto AddRepairRow = [&](const FRubezhInventoryEntry& Entry, TFunction<void()> Request)
	{
		const FRubezhItemDefinition* Item = Inventory.FindDefinition(Entry.ItemId);
		const bool bRepairable = Item && (Kind == ERubezhTraderKind::Gunsmith ? Rubezh::Items::FindWeapon(Entry.ItemId) != nullptr : Rubezh::Equipment::CanWear(*Item));
		const int32 Cost = bRepairable ? Rubezh::Trade::GetRepairCost(Entry, *Item) : 0;
		if (Cost <= 0)
		{
			return;
		}
		if (!bHasHeader)
		{
			AddHeader(Kind == ERubezhTraderKind::Gunsmith ? TEXT("Ремонт оружия") : TEXT("Ремонт снаряжения"));
			bHasHeader = true;
		}

		const FString Label = FString::Printf(TEXT("%s — %s"), *DescribeWorn(*Item, Entry), *Rubezh::Trade::FormatMoney(Cost));
		const int32 Money = Inventory.GetMoney();
		AddRow(Label, TEXT("Починить"), [this, Cost, Money, Request = MoveTemp(Request)]
		{
			if (Cost > Money)
			{
				ShowError(Rubezh::Trade::Describe(Rubezh::Trade::EResult::NotEnoughMoney));
				return;
			}
			Request();
		});
	};

	URubezhInventoryComponent* InventoryPtr = &Inventory;
	if (Kind == ERubezhTraderKind::Gunsmith)
	{
		const Rubezh::Weapons::FWeaponSlots& Slots = Inventory.GetWeaponSlots();
		for (int32 WeaponSlot = 0; WeaponSlot < Slots.Num(); ++WeaponSlot)
		{
			if (!Slots[WeaponSlot].ItemId.IsNone())
			{
				AddRepairRow(Slots[WeaponSlot], [InventoryPtr, WeaponSlot] { InventoryPtr->ServerRepairWeapon(WeaponSlot); });
			}
		}
	}
	else
	{
		for (const ERubezhEquipSlot EquipSlot : Rubezh::Equipment::AllSlots)
		{
			FRubezhInventoryEntry Worn;
			Worn.ItemId = Inventory.GetEquipped(EquipSlot);
			Worn.Count = 1;
			Worn.Durability = Inventory.GetEquippedDurability(EquipSlot);
			if (!Worn.ItemId.IsNone())
			{
				AddRepairRow(Worn, [InventoryPtr, EquipSlot] { InventoryPtr->ServerRepairEquipped(EquipSlot); });
			}
		}
	}

	const TArray<FRubezhInventoryEntry>& Entries = Inventory.GetGrid().GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FName ItemId = Entries[Index].ItemId;
		const int32 Cell = Entries[Index].Slot;
		AddRepairRow(Entries[Index], [InventoryPtr, Cell, ItemId] { InventoryPtr->ServerRepairEntry(Cell, ItemId); });
	}
}

void URubezhTraderPanel::RequestBuy(URubezhInventoryComponent& Inventory, FName ItemId, int32 Count, ERubezhTraderKind Kind)
{
	// Проверяем у себя теми же правилами, что и сервер, — чтобы сразу сказать, почему нельзя.
	FRubezhInventoryGrid Trial = Inventory.GetGrid();
	int32 TrialMoney = Inventory.GetMoney();
	const Rubezh::Trade::EResult Result = Rubezh::Trade::Buy(Trial, TrialMoney, ItemId, Count, Kind, [&Inventory](FName Id) { return Inventory.FindDefinition(Id); });
	if (Result != Rubezh::Trade::EResult::Ok)
	{
		ShowError(Rubezh::Trade::Describe(Result));
		return;
	}
	Inventory.ServerBuyItem(ItemId, Count);
}

void URubezhTraderPanel::ShowError(const FString& Message)
{
	ErrorText->SetText(FText::FromString(Message));
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ErrorTimer, this, &ThisClass::ClearError, Rubezh::InventoryStyle::ErrorShowSeconds, false);
	}
}

void URubezhTraderPanel::ClearError()
{
	ErrorText->SetText(FText::GetEmpty());
}
