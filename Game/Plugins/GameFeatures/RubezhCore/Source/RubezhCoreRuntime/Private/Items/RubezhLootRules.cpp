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

	int32 ComputeAmmoLoss(int32 Total, float Percent)
	{
		return FMath::Clamp(FMath::RoundToInt32(FMath::Max(Total, 0) * Percent / 100.0f), 0, FMath::Max(Total, 0));
	}

	namespace
	{
		/** Калибр, которым заряжен магазин; None — это не заряженный магазин или его патроны неизвестны. */
		FName GetLoadedCalibre(const FRubezhInventoryEntry& Entry, FRubezhItemLookup Lookup)
		{
			const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
			const bool bLoadedMagazine = Definition && Definition->MagazineCapacity > 0 && Entry.Rounds > 0;
			return bLoadedMagazine && Lookup(Definition->AmmoItem) ? Definition->AmmoItem : NAME_None;
		}

		/** Вынуть патроны из магазинов этого калибра, начиная с наименее полного. */
		void TakeRoundsFromMagazines(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup, FName Calibre, int32 Count)
		{
			while (Count > 0)
			{
				FRubezhInventoryEntry* Lightest = nullptr;
				for (int32 Index = 0; Index < Grid.GetEntries().Num(); ++Index)
				{
					FRubezhInventoryEntry* Entry = Grid.GetEntryMutable(Index);
					if (GetLoadedCalibre(*Entry, Lookup) == Calibre && (!Lightest || Entry->Rounds < Lightest->Rounds))
					{
						Lightest = Entry;
					}
				}
				if (!Lightest)
				{
					return;
				}

				const int32 Taken = FMath::Min(Lightest->Rounds, Count);
				Lightest->Rounds -= Taken;
				Count -= Taken;
			}
		}
	}

	TArray<FDrop> TakeAmmoLoss(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup, float Percent)
	{
		TArray<FDrop> Calibres;
		const auto AddRounds = [&Calibres](FName Calibre, int32 Rounds)
		{
			FDrop* Known = Calibres.FindByPredicate([Calibre](const FDrop& Drop) { return Drop.ItemId == Calibre; });
			if (Known)
			{
				Known->Count += Rounds;
			}
			else
			{
				Calibres.Add({ Calibre, Rounds });
			}
		};

		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
			const FName LoadedCalibre = GetLoadedCalibre(Entry, Lookup);
			if (Definition && Definition->Category == ERubezhItemCategory::Ammo)
			{
				AddRounds(Entry.ItemId, Entry.Count);
			}
			else if (!LoadedCalibre.IsNone())
			{
				AddRounds(LoadedCalibre, Entry.Rounds);
			}
		}

		TArray<FDrop> Lost;
		for (const FDrop& Calibre : Calibres)
		{
			const int32 Count = ComputeAmmoLoss(Calibre.Count, Percent);
			if (Count == 0)
			{
				continue;
			}

			const int32 Loose = Grid.RemoveItem(Calibre.ItemId, Count);
			TakeRoundsFromMagazines(Grid, Lookup, Calibre.ItemId, Count - Loose);
			Lost.Add({ Calibre.ItemId, Count });
		}
		return Lost;
	}

	FRubezhInventoryGrid TakeLostOnDeath(FRubezhInventoryGrid& Grid, FRubezhItemLookup Lookup, FRandomStream& Random)
	{
		// Неизвестный предмет (удалён из таблицы) остаётся у игрока — не теряем его молча.
		const auto RollDrop = [&Random]() { return Random.FRandRange(0.0f, 100.0f) < DeathDropChancePercent; };
		TArray<FRubezhInventoryEntry> Lost = Grid.ExtractEntries([&Lookup, &RollDrop](const FRubezhInventoryEntry& Entry)
		{
			const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
			return Definition && Rubezh::Items::IsLostOnDeath(Definition->Category) && RollDrop();
		});

		if (RollDrop())
		{
			const float Percent = Random.FRandRange(DeathAmmoLossMinPercent, DeathAmmoLossMaxPercent);
			for (const FDrop& Drop : TakeAmmoLoss(Grid, Lookup, Percent))
			{
				// Выпавшее может не влезть в одну стопку: на теле столько ячеек, сколько стопок.
				const int32 MaxStack = FMath::Max(Lookup(Drop.ItemId)->MaxStack, 1);
				for (int32 Remaining = Drop.Count; Remaining > 0; Remaining -= MaxStack)
				{
					FRubezhInventoryEntry Stack;
					Stack.ItemId = Drop.ItemId;
					Stack.Count = FMath::Min(Remaining, MaxStack);
					Lost.Add(Stack);
				}
			}
		}
		return FRubezhInventoryGrid::FromEntries(Lost);
	}
}
