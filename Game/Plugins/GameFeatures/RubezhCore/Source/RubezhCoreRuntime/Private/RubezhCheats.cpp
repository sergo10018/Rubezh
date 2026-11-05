// РУБЕЖ: Земля после разлома
// Отладочные консольные команды для плейтеста. В сборку Shipping не попадают.

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"

#if !UE_BUILD_SHIPPING

namespace
{
	URubezhInventoryComponent* FindLocalInventory(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return URubezhInventoryComponent::FindForActor(Controller);
	}

	void GiveItem(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Give <ItemId> [Count] — например: Rubezh.Give ammo_762x39 60"));
			return;
		}
		if (!World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Give работает только там, где есть сервер (одиночная игра или хост)"));
			return;
		}

		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Give: у игрока нет рюкзака"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const int32 NotAdded = Inventory->AddItem(FName(*Args[0]), Count);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Give %s ×%d: добавлено %d, не влезло %d, вес %.2f кг"),
			*Args[0], Count, Count - NotAdded, NotAdded, Inventory->GetTotalWeight());
	}

	void TakeItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		if (!Inventory || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Take <ItemId> [Count] — только в одиночной игре или на хосте"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const int32 Removed = Inventory->RemoveItem(FName(*Args[0]), Count);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Take %s: убрано %d, вес %.2f кг"), *Args[0], Removed, Inventory->GetTotalWeight());
	}

	void SpawnItem(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* Controller = (World && World->GetNetMode() != NM_Client) ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Spawn <ItemId> [Count] — положить предмет на землю перед собой (одиночная игра или хост)"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const ARubezhWorldItem* Item = ARubezhWorldItem::SpawnInFrontOf(*Pawn, FName(*Args[0]), Count);
		if (Item && !Item->FindDefinition())
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Spawn: предмета '%s' нет в таблице — лежит серая коробка"), *Args[0]);
		}
	}

	void PrintInventory(const TArray<FString>& Args, UWorld* World)
	{
		const URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Inventory: у игрока нет рюкзака"));
			return;
		}

		const FRubezhInventoryGrid& Grid = Inventory->GetGrid();
		UE_LOG(LogRubezh, Display, TEXT("Рюкзак %dx%d, стопок %d, вес %.2f кг"),
			Grid.GetSize().X, Grid.GetSize().Y, Grid.GetEntries().Num(), Inventory->GetTotalWeight());
		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			UE_LOG(LogRubezh, Display, TEXT("  %s ×%d в (%d,%d)%s"),
				*Entry.ItemId.ToString(), Entry.Count, Entry.Position.X, Entry.Position.Y, Entry.bRotated ? TEXT(" повёрнут") : TEXT(""));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GiveCommand(
		TEXT("Rubezh.Give"), TEXT("Положить предмет в рюкзак: Rubezh.Give <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveItem));

	FAutoConsoleCommandWithWorldAndArgs TakeCommand(
		TEXT("Rubezh.Take"), TEXT("Убрать предмет из рюкзака: Rubezh.Take <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TakeItem));

	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(
		TEXT("Rubezh.Spawn"), TEXT("Положить предмет на землю перед собой: Rubezh.Spawn <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnItem));

	FAutoConsoleCommandWithWorldAndArgs InventoryCommand(
		TEXT("Rubezh.Inventory"), TEXT("Показать содержимое рюкзака и вес"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintInventory));
}

#endif // !UE_BUILD_SHIPPING
