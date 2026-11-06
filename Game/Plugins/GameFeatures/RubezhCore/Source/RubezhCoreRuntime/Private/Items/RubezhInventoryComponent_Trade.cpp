// РУБЕЖ: Земля после разлома
// URubezhInventoryComponent: деньги, сделки с торговцами, ремонт и чистка оружия. Всё решает сервер; правила — Items/RubezhTradeRules.h.

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhPickupRules.h"
#include "Items/RubezhTradeRules.h"
#include "RubezhCoreRuntimeModule.h"
#include "Settlement/RubezhTrader.h"
#include "TimerManager.h"

namespace
{
	// Как часто сервер проверяет, что игрок не ушёл от торговца.
	constexpr float TraderReachCheckSeconds = 0.5f;
}

bool URubezhInventoryComponent::CanReachTrader(const ARubezhTrader& Trader) const
{
	const APawn* Pawn = GetOwnerPawn();
	return Pawn && !IsOwnerDead() && IsValid(&Trader) && Rubezh::Pickup::IsWithinReach(Pawn->GetActorLocation(), Trader.GetFocusLocation(), true);
}

const ARubezhTrader* URubezhInventoryComponent::GetReachableTrader() const
{
	return (TraderInUse && CanReachTrader(*TraderInUse)) ? TraderInUse.Get() : nullptr;
}

void URubezhInventoryComponent::ServerOpenTrader_Implementation(ARubezhTrader* Trader)
{
	if (!IsValid(Trader) || !CanReachTrader(*Trader) || TraderInUse == Trader)
	{
		return;
	}

	CloseLoot();
	CancelSearch();
	TraderInUse = Trader;
	GetWorld()->GetTimerManager().SetTimer(TraderReachTimer, this, &ThisClass::CheckTraderReach, TraderReachCheckSeconds, true);

	OnInventoryChanged.Broadcast();
	OpenWindowForLocalPlayer();
}

void URubezhInventoryComponent::ServerCloseTrader_Implementation()
{
	CloseTrader();
}

void URubezhInventoryComponent::CloseTrader()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraderReachTimer);
	}
	if (TraderInUse)
	{
		TraderInUse = nullptr;
		OnInventoryChanged.Broadcast();
	}
}

void URubezhInventoryComponent::CheckTraderReach()
{
	if (TraderInUse && !CanReachTrader(*TraderInUse))
	{
		CloseTrader();
	}
}

void URubezhInventoryComponent::OnRep_TraderInUse(ARubezhTrader* PreviousTrader)
{
	OnInventoryChanged.Broadcast();
	if (TraderInUse && !PreviousTrader)
	{
		OpenWindowForLocalPlayer();
	}
}

void URubezhInventoryComponent::OnRep_Money()
{
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::SetMoney(int32 NewMoney)
{
	Money = FMath::Max(NewMoney, 0);
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::ServerBuyItem_Implementation(FName ItemId, int32 Count)
{
	const ARubezhTrader* Trader = GetReachableTrader();
	if (!Trader)
	{
		return;
	}

	const Rubezh::Trade::EResult Result = Rubezh::Trade::Buy(Grid, Money, ItemId, Count, Trader->GetKind(), [this](FName Id) { return FindDefinition(Id); });
	LogTrade(TEXT("покупка"), ItemId, Result);
	if (Result == Rubezh::Trade::EResult::Ok)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerSellEntry_Implementation(int32 Cell, FName ItemId)
{
	const ARubezhTrader* Trader = GetReachableTrader();
	// Номер ячейки, а не индекс стопки: после продажи индексы сдвигаются, и второй быстрый клик продал бы соседний такой же предмет.
	const int32 EntryIndex = Grid.FindEntryAt(Cell);
	if (!Trader || EntryIndex == INDEX_NONE || Grid.GetEntries()[EntryIndex].ItemId != ItemId)
	{
		return;
	}

	const Rubezh::Trade::EResult Result = Rubezh::Trade::Sell(Grid, Money, EntryIndex, Trader->GetKind(),
		[this](FName Id) { return FindDefinition(Id); }, [](FName Id) { return Rubezh::Items::FindWeapon(Id); });
	LogTrade(TEXT("продажа"), ItemId, Result);
	if (Result == Rubezh::Trade::EResult::Ok)
	{
		// Иначе набивка перейдёт на магазин, который позже ляжет в освободившуюся ячейку.
		ServerStopLoadingMagazine_Implementation();
		HandleGridChanged();
	}
}

bool URubezhInventoryComponent::CanRepairHere(FName ItemId) const
{
	const ARubezhTrader* Trader = GetReachableTrader();
	const FRubezhItemDefinition* Item = FindDefinition(ItemId);
	if (!Trader || !Item)
	{
		return false;
	}
	// Оружие чинит оружейник, броню и одежду — бронник.
	return Trader->GetKind() == ERubezhTraderKind::Gunsmith ? Rubezh::Items::FindWeapon(ItemId) != nullptr
		: Trader->GetKind() == ERubezhTraderKind::Armorer && Rubezh::Equipment::CanWear(*Item);
}

void URubezhInventoryComponent::ServerRepairEntry_Implementation(int32 Cell, FName ItemId)
{
	FRubezhInventoryEntry* Entry = Grid.GetEntryMutable(Grid.FindEntryAt(Cell));
	if (Entry && Entry->ItemId == ItemId && CanRepairHere(ItemId))
	{
		RepairEntry(*Entry);
	}
}

void URubezhInventoryComponent::ServerRepairWeapon_Implementation(int32 WeaponSlot)
{
	if (WeaponSlots.IsValidIndex(WeaponSlot) && CanRepairHere(WeaponSlots[WeaponSlot].ItemId))
	{
		RepairEntry(WeaponSlots[WeaponSlot]);
	}
}

void URubezhInventoryComponent::ServerRepairEquipped_Implementation(ERubezhEquipSlot Slot)
{
	const FName ItemId = GetEquipped(Slot);
	if (ItemId.IsNone() || !CanRepairHere(ItemId))
	{
		return;
	}

	FRubezhInventoryEntry Entry;
	Entry.ItemId = ItemId;
	Entry.Count = 1;
	Entry.Durability = GetEquippedDurability(Slot);
	RepairEntry(Entry);
	SetEquippedDurability(Slot, Entry.Durability);
}

void URubezhInventoryComponent::RepairEntry(FRubezhInventoryEntry& Entry)
{
	const FRubezhItemDefinition* Item = FindDefinition(Entry.ItemId);
	if (!Item)
	{
		return;
	}

	const Rubezh::Trade::EResult Result = Rubezh::Trade::Repair(Entry, Money, *Item);
	LogTrade(TEXT("ремонт"), Entry.ItemId, Result);
	if (Result == Rubezh::Trade::EResult::Ok)
	{
		// Осечка следующего выстрела была решена для старой прочности.
		if (Rubezh::Items::FindWeapon(Entry.ItemId))
		{
			RollNextShot(Entry);
		}
		HandleGridChanged();
	}
}

bool URubezhInventoryComponent::CanCleanWeapon(int32 WeaponSlot) const
{
	return WeaponSlots.IsValidIndex(WeaponSlot) && !WeaponSlots[WeaponSlot].ItemId.IsNone() && Rubezh::Trade::CanClean(WeaponSlots[WeaponSlot].Durability);
}

FName URubezhInventoryComponent::GetWeaponInSlot(int32 WeaponSlot) const
{
	return WeaponSlots.IsValidIndex(WeaponSlot) ? WeaponSlots[WeaponSlot].ItemId : NAME_None;
}

bool URubezhInventoryComponent::CleanWeapon(int32 WeaponSlot, float CleanPercent)
{
	if (!CanCleanWeapon(WeaponSlot))
	{
		return false;
	}
	SetWeaponDurability(WeaponSlot, Rubezh::Trade::GetCleanedDurability(WeaponSlots[WeaponSlot].Durability, CleanPercent));
	return true;
}

void URubezhInventoryComponent::LogTrade(const TCHAR* Action, FName ItemId, Rubezh::Trade::EResult Result) const
{
	// Каждая сделка — в журнал: без этого дюп или ошибку цены потом не найти.
	UE_LOG(LogRubezh, Log, TEXT("%s: %s '%s' — %s, на счёте %d"), *GetNameSafe(GetOwner()), Action, *ItemId.ToString(), Rubezh::Trade::Describe(Result), Money);
}
