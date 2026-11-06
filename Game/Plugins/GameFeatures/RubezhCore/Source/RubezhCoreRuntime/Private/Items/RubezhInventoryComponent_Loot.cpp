// РУБЕЖ: Земля после разлома
// URubezhInventoryComponent: обыск ящиков, тел и брошенных рюкзаков, перенос вещей, выпадение лута при смерти.

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhLootContainer.h"
#include "Items/RubezhLootRules.h"
#include "Items/RubezhPickupRules.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "TimerManager.h"
#include "UI/RubezhPlayerInput.h"

namespace
{
	// Как часто сервер проверяет, что обыскивающий не ушёл от ящика.
	constexpr float LootReachCheckSeconds = 0.5f;

	TOptional<int32> ToOptionalSlot(int32 Slot)
	{
		return Slot == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(Slot);
	}

	bool EntryMatches(const FRubezhInventoryGrid& Grid, int32 EntryIndex, FName ItemId)
	{
		return Grid.GetEntries().IsValidIndex(EntryIndex) && Grid.GetEntries()[EntryIndex].ItemId == ItemId;
	}
}

double URubezhInventoryComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

bool URubezhInventoryComponent::IsSearching(const ARubezhLootContainer* Container) const
{
	return Container && SearchingContainer == Container;
}

float URubezhInventoryComponent::GetSearchSecondsLeft() const
{
	return SearchingContainer ? FMath::Max(0.0f, static_cast<float>(SearchFinishTime - GetServerTime())) : 0.0f;
}

bool URubezhInventoryComponent::CanReach(const ARubezhLootContainer& Container) const
{
	// Клиенту не верим: расстояние проверяет сервер (с запасом на задержку сети).
	const APawn* Pawn = GetOwnerPawn();
	return Pawn && IsValid(&Container) && Rubezh::Pickup::IsWithinReach(Pawn->GetActorLocation(), Container.GetFocusLocation(), true);
}

ARubezhLootContainer* URubezhInventoryComponent::GetReachableLoot() const
{
	return (LootContainer && CanReach(*LootContainer)) ? LootContainer.Get() : nullptr;
}

void URubezhInventoryComponent::ServerStartSearch_Implementation(ARubezhLootContainer* Container)
{
	if (!IsValid(Container) || !CanReach(*Container) || LootContainer == Container || IsSearching(Container))
	{
		return;
	}

	CloseLoot();
	CancelSearch();
	CloseTrader();
	if (Container->WasSearchedBy(*this) || Container->GetSearchSeconds() <= 0.0f)
	{
		OpenLoot(*Container);
		return;
	}

	SearchingContainer = Container;
	SearchFinishTime = GetServerTime() + Container->GetSearchSeconds();
	GetWorld()->GetTimerManager().SetTimer(SearchTimer, this, &ThisClass::FinishSearch, Container->GetSearchSeconds(), false);
}

void URubezhInventoryComponent::FinishSearch()
{
	ARubezhLootContainer* Container = SearchingContainer;
	CancelSearch();
	if (IsValid(Container) && CanReach(*Container))
	{
		Container->MarkSearchedBy(*this);
		OpenLoot(*Container);
	}
}

void URubezhInventoryComponent::CancelSearch()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTimer);
	}
	SearchingContainer = nullptr;
	SearchFinishTime = 0.0;
}

void URubezhInventoryComponent::OpenLoot(ARubezhLootContainer& Container)
{
	LootContainer = &Container;
	LootGrid = Container.GetContents();
	Container.AddViewer(*this);
	GetWorld()->GetTimerManager().SetTimer(LootReachTimer, this, &ThisClass::CheckLootReach, LootReachCheckSeconds, true);

	OnInventoryChanged.Broadcast();
	OpenWindowForLocalPlayer();
}

void URubezhInventoryComponent::CheckLootReach()
{
	if (LootContainer && !CanReach(*LootContainer))
	{
		CloseLoot();
	}
}

void URubezhInventoryComponent::CloseLoot()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootReachTimer);
	}
	if (!LootContainer && LootGrid.GetSlotCount() == 0)
	{
		return;
	}

	if (LootContainer)
	{
		LootContainer->RemoveViewer(*this);
	}
	LootContainer = nullptr;
	LootGrid = FRubezhInventoryGrid();
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::RefreshLootView()
{
	if (LootContainer)
	{
		LootGrid = LootContainer->GetContents();
		OnInventoryChanged.Broadcast();
	}
}

void URubezhInventoryComponent::OnRep_LootContainer(ARubezhLootContainer* PreviousContainer)
{
	OnInventoryChanged.Broadcast();
	if (LootContainer && !PreviousContainer)
	{
		OpenWindowForLocalPlayer();
	}
}

void URubezhInventoryComponent::OpenWindowForLocalPlayer() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	APlayerController* Controller = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	URubezhPlayerInput* PlayerInput = GameInstance ? GameInstance->GetSubsystem<URubezhPlayerInput>() : nullptr;
	if (Controller && Controller->IsLocalController() && PlayerInput)
	{
		PlayerInput->OpenInventory(Controller);
	}
}

void URubezhInventoryComponent::ServerCloseLoot_Implementation()
{
	CloseLoot();
}

void URubezhInventoryComponent::ServerTakeLoot_Implementation(int32 LootEntryIndex, FName ItemId, int32 TargetSlot)
{
	ARubezhLootContainer* Container = GetReachableLoot();
	if (!Container)
	{
		return;
	}

	int32 Moved = 0;
	Container->ModifyContents([&](FRubezhInventoryGrid& Contents)
	{
		if (!EntryMatches(Contents, LootEntryIndex, ItemId))
		{
			return false;
		}
		Moved = Rubezh::Loot::Transfer(Contents, LootEntryIndex, Grid, ToOptionalSlot(TargetSlot), [this](FName Id) { return FindDefinition(Id); });
		return Moved > 0;
	});
	if (Moved > 0)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerPutLoot_Implementation(int32 EntryIndex, FName ItemId, int32 TargetSlot)
{
	ARubezhLootContainer* Container = GetReachableLoot();
	if (!Container || !EntryMatches(Grid, EntryIndex, ItemId))
	{
		return;
	}

	int32 Moved = 0;
	Container->ModifyContents([&](FRubezhInventoryGrid& Contents)
	{
		Moved = Rubezh::Loot::Transfer(Grid, EntryIndex, Contents, ToOptionalSlot(TargetSlot), [this](FName Id) { return FindDefinition(Id); });
		return Moved > 0;
	});
	if (Moved > 0)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerMoveLootEntry_Implementation(int32 LootEntryIndex, FName ItemId, int32 NewSlot)
{
	if (ARubezhLootContainer* Container = GetReachableLoot())
	{
		Container->ModifyContents([&](FRubezhInventoryGrid& Contents)
		{
			return EntryMatches(Contents, LootEntryIndex, ItemId)
				&& Contents.MoveEntry(LootEntryIndex, NewSlot, [this](FName Id) { return FindDefinition(Id); });
		});
	}
}

void URubezhInventoryComponent::ServerTakeAllLoot_Implementation()
{
	ARubezhLootContainer* Container = GetReachableLoot();
	if (!Container)
	{
		return;
	}

	int32 Moved = 0;
	Container->ModifyContents([&](FRubezhInventoryGrid& Contents)
	{
		// С конца — чтобы индексы ещё не перенесённых стопок не сдвигались.
		for (int32 Index = Contents.GetEntries().Num() - 1; Index >= 0; --Index)
		{
			Moved += Rubezh::Loot::Transfer(Contents, Index, Grid, {}, [this](FName Id) { return FindDefinition(Id); });
		}
		return Moved > 0;
	});
	if (Moved > 0)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerWearLootBag_Implementation()
{
	ARubezhLootContainer* Container = GetReachableLoot();
	if (!Container || Container->GetBagItemId().IsNone())
	{
		return;
	}

	FRubezhInventoryGrid NewGrid = Grid;
	Rubezh::Equipment::FEquipped NewEquipped = Equipped;
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::WearBag(NewGrid, NewEquipped, Container->GetBagItemId(),
		Container->GetContents(), [this](FName Id) { return FindDefinition(Id); });
	LogEquipFailure(Result);
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		return;
	}

	Grid = MoveTemp(NewGrid);
	Equipped = MoveTemp(NewEquipped);
	Container->RemoveAsWornBag();	// закроет обыск у всех, кто смотрел
	HandleGridChanged();
}

void URubezhInventoryComponent::DropBodyOnDeath(const APawn& Pawn)
{
	// Сначала кладём на землю, потом убираем из рюкзака — если положить не вышло, вещи не пропадут.
	FRubezhInventoryGrid Kept = Grid;
	FRandomStream Random(FMath::Rand());
	const FRubezhInventoryGrid Lost = Rubezh::Loot::TakeLostOnDeath(Kept, [this](FName Id) { return FindDefinition(Id); }, Random);
	if (Lost.IsEmpty())
	{
		return;
	}

	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const FText Name = FText::FromString(FString::Printf(TEXT("Тело: %s"), PlayerState ? *PlayerState->GetPlayerName() : TEXT("неизвестный")));
	const FVector Location = ARubezhWorldItem::FindGround(*GetWorld(), Pawn.GetActorLocation(), &Pawn);
	if (!ARubezhLootContainer::SpawnDropped(*GetWorld(), Location, Pawn.GetActorRotation().Yaw, Name, Lost, NAME_None,
		GetDefault<URubezhSettings>()->BodyLifetimeSeconds))
	{
		UE_LOG(LogRubezh, Error, TEXT("%s: не удалось выложить вещи на тело — они остались в рюкзаке"), *GetNameSafe(GetOwner()));
		return;
	}

	Grid = MoveTemp(Kept);
	HandleGridChanged();
}
