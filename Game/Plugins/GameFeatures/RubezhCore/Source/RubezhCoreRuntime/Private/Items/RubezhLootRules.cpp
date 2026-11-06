// РУБЕЖ: Земля после разлома

#include "Items/RubezhLootRules.h"

#include "Items/RubezhItemCategory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhLootRules)

namespace Rubezh::Loot
{
	TArray<FDrop> Roll(TConstArrayView<FRubezhLootTableRow> Rows, FName Table, FRandomStream& Random)
	{
		TArray<FDrop> Drops;
		for (const FRubezhLootTableRow& Row : Rows)
		{
			if (Row.Table != Table || Row.ItemId.IsNone() || Random.FRandRange(0.0f, 100.0f) >= Row.Chance)
			{
				continue;
			}

			const int32 MinCount = FMath::Max(Row.MinCount, 1);
			const int32 MaxCount = FMath::Max(Row.MaxCount, MinCount);
			Drops.Add({ Row.ItemId, Random.RandRange(MinCount, MaxCount) });
		}
		return Drops;
	}

	int32 Transfer(FRubezhInventoryGrid& From, int32 EntryIndex, FRubezhInventoryGrid& To, TOptional<int32> TargetSlot, FRubezhItemLookup Lookup)
	{
		if (!From.GetEntries().IsValidIndex(EntryIndex))
		{
			return 0;
		}
		const FRubezhInventoryEntry Entry = From.GetEntries()[EntryIndex];
		const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
		if (!Definition)
		{
			return 0;
		}

		int32 Moved = 0;
		const int32 TargetIndex = TargetSlot.IsSet() ? To.FindEntryAt(TargetSlot.GetValue()) : INDEX_NONE;
		const bool bSameItemThere = TargetIndex != INDEX_NONE && To.GetEntries()[TargetIndex].ItemId == Entry.ItemId;
		if (bSameItemThere)
		{
			// На такую же стопку — докладываем, сколько влезет; остальное остаётся на месте.
			Moved = To.AddToEntry(TargetIndex, Entry.Count, Definition->MaxStack);
		}
		else if (TargetSlot.IsSet() && TargetIndex == INDEX_NONE && To.PlaceEntry(Entry, TargetSlot.GetValue()))
		{
			Moved = Entry.Count;
		}
		else
		{
			// Занятая другим предметом ячейка или «куда влезет»: докладываем в стопки, потом в пустые ячейки.
			Moved = Entry.Count - To.AddEntry(Entry, *Definition);
		}

		From.ReduceEntry(EntryIndex, Moved);
		return Moved;
	}

	FRubezhInventoryGrid TakeLostOnDeath(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup)
	{
		// Неизвестный предмет (удалён из таблицы) остаётся у игрока — не теряем его молча.
		return FRubezhInventoryGrid::FromEntries(Grid.ExtractEntries([&Lookup](const FRubezhInventoryEntry& Entry)
		{
			const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
			return Definition && Rubezh::Items::IsLostOnDeath(Definition->Category);
		}));
	}
}
