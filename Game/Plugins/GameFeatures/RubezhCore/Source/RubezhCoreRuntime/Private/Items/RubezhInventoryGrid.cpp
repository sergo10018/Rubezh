// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryGrid.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryGrid)

int32 FRubezhInventoryGrid::FindEntryAt(int32 Slot) const
{
	return Entries.IndexOfByPredicate([Slot](const FRubezhInventoryEntry& Entry) { return Entry.Slot == Slot; });
}

int32 FRubezhInventoryGrid::FindFreeSlot() const
{
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		if (FindEntryAt(Slot) == INDEX_NONE)
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

int32 FRubezhInventoryGrid::AddItem(FName ItemId, const FRubezhItemDefinition& Definition, int32 Count)
{
	int32 Remaining = FMath::Max(Count, 0);
	const int32 MaxStack = FMath::Max(Definition.MaxStack, 1);

	for (FRubezhInventoryEntry& Entry : Entries)
	{
		if (Remaining == 0)
		{
			break;
		}
		if (Entry.ItemId == ItemId && Entry.Count < MaxStack)
		{
			const int32 Added = FMath::Min(MaxStack - Entry.Count, Remaining);
			Entry.Count += Added;
			Remaining -= Added;
		}
	}

	while (Remaining > 0)
	{
		const int32 Slot = FindFreeSlot();
		if (Slot == INDEX_NONE)
		{
			break;
		}

		FRubezhInventoryEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.ItemId = ItemId;
		Entry.Count = FMath::Min(MaxStack, Remaining);
		Entry.Slot = Slot;
		Remaining -= Entry.Count;
	}

	return Remaining;
}

int32 FRubezhInventoryGrid::RemoveItem(FName ItemId, int32 Count)
{
	int32 Removed = 0;
	for (int32 Index = Entries.Num() - 1; Index >= 0 && Removed < Count; --Index)
	{
		FRubezhInventoryEntry& Entry = Entries[Index];
		if (Entry.ItemId != ItemId)
		{
			continue;
		}

		const int32 Taken = FMath::Min(Entry.Count, Count - Removed);
		Entry.Count -= Taken;
		Removed += Taken;
		if (Entry.Count == 0)
		{
			Entries.RemoveAt(Index);
		}
	}
	return Removed;
}

bool FRubezhInventoryGrid::RemoveEntry(int32 EntryIndex, FRubezhInventoryEntry& OutRemoved)
{
	if (!Entries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	OutRemoved = Entries[EntryIndex];
	Entries.RemoveAt(EntryIndex);
	return true;
}

int32 FRubezhInventoryGrid::AddToEntry(int32 EntryIndex, int32 Count, int32 MaxStack)
{
	if (!Entries.IsValidIndex(EntryIndex) || Count <= 0)
	{
		return 0;
	}

	const int32 Added = FMath::Clamp(FMath::Max(MaxStack, 1) - Entries[EntryIndex].Count, 0, Count);
	Entries[EntryIndex].Count += Added;
	return Added;
}

bool FRubezhInventoryGrid::ReduceEntry(int32 EntryIndex, int32 Count)
{
	if (!Entries.IsValidIndex(EntryIndex) || Count < 0 || Entries[EntryIndex].Count < Count)
	{
		return false;
	}

	Entries[EntryIndex].Count -= Count;
	if (Entries[EntryIndex].Count == 0)
	{
		Entries.RemoveAt(EntryIndex);
	}
	return true;
}

bool FRubezhInventoryGrid::MoveEntry(int32 EntryIndex, int32 NewSlot, FRubezhItemLookup Lookup)
{
	if (!Entries.IsValidIndex(EntryIndex) || NewSlot < 0 || NewSlot >= SlotCount)
	{
		return false;
	}

	FRubezhInventoryEntry& Moved = Entries[EntryIndex];
	const int32 TargetIndex = FindEntryAt(NewSlot);
	if (TargetIndex == INDEX_NONE || TargetIndex == EntryIndex)
	{
		Moved.Slot = NewSlot;
		return true;
	}

	FRubezhInventoryEntry& Target = Entries[TargetIndex];
	const FRubezhItemDefinition* Definition = Lookup(Moved.ItemId);
	const int32 MaxStack = Definition ? FMath::Max(Definition->MaxStack, 1) : 1;
	if (Target.ItemId == Moved.ItemId && Target.Count < MaxStack)
	{
		return ReduceEntry(EntryIndex, AddToEntry(TargetIndex, Moved.Count, MaxStack));
	}

	// Разные предметы (или полная стопка) — меняем местами.
	Target.Slot = Moved.Slot;
	Moved.Slot = NewSlot;
	return true;
}

bool FRubezhInventoryGrid::AddEntryAt(FName ItemId, int32 Count, int32 Slot, FRubezhItemLookup Lookup)
{
	const FRubezhItemDefinition* Definition = Lookup(ItemId);
	if (!Definition || Count <= 0 || Count > FMath::Max(Definition->MaxStack, 1) || !IsSlotFree(Slot))
	{
		return false;
	}

	FRubezhInventoryEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.ItemId = ItemId;
	Entry.Count = Count;
	Entry.Slot = Slot;
	return true;
}

bool FRubezhInventoryGrid::PlaceEntry(const FRubezhInventoryEntry& Entry, int32 Slot)
{
	if (!IsSlotFree(Slot))
	{
		return false;
	}

	FRubezhInventoryEntry& Placed = Entries.Add_GetRef(Entry);
	Placed.Slot = Slot;
	return true;
}

int32 FRubezhInventoryGrid::AddEntry(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Definition)
{
	if (Definition.MaxStack > 1)
	{
		return AddItem(Entry.ItemId, Definition, Entry.Count);
	}
	return PlaceEntry(Entry, FindFreeSlot()) ? 0 : Entry.Count;
}

bool FRubezhInventoryGrid::Resize(int32 NewSlotCount)
{
	if (NewSlotCount < 0 || Entries.Num() > NewSlotCount)
	{
		return false;
	}

	FRubezhInventoryGrid Result(NewSlotCount);
	TArray<const FRubezhInventoryEntry*> Displaced;
	for (const FRubezhInventoryEntry& Entry : Entries)
	{
		if (Entry.Slot < NewSlotCount)
		{
			Result.Entries.Add(Entry);
		}
		else
		{
			Displaced.Add(&Entry);
		}
	}

	for (const FRubezhInventoryEntry* Entry : Displaced)
	{
		FRubezhInventoryEntry& Placed = Result.Entries.Add_GetRef(*Entry);
		Placed.Slot = Result.FindFreeSlot();
	}

	*this = MoveTemp(Result);
	return true;
}

TArray<FRubezhInventoryEntry> FRubezhInventoryGrid::ExtractEntries(TFunctionRef<bool(const FRubezhInventoryEntry&)> Predicate)
{
	TArray<FRubezhInventoryEntry> Extracted;
	for (int32 Index = 0; Index < Entries.Num();)
	{
		if (Predicate(Entries[Index]))
		{
			Extracted.Add(Entries[Index]);
			Entries.RemoveAt(Index);
		}
		else
		{
			++Index;
		}
	}
	return Extracted;
}

FRubezhInventoryGrid FRubezhInventoryGrid::FromEntries(const TArray<FRubezhInventoryEntry>& InEntries)
{
	FRubezhInventoryGrid Result(InEntries.Num());
	Result.Entries = InEntries;
	for (int32 Index = 0; Index < Result.Entries.Num(); ++Index)
	{
		Result.Entries[Index].Slot = Index;
	}
	return Result;
}

int32 FRubezhInventoryGrid::CountItem(FName ItemId) const
{
	int32 Total = 0;
	for (const FRubezhInventoryEntry& Entry : Entries)
	{
		if (Entry.ItemId == ItemId)
		{
			Total += Entry.Count;
		}
	}
	return Total;
}

float FRubezhInventoryGrid::GetLoadedRoundsWeight(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Definition, FRubezhItemLookup Lookup)
{
	const FRubezhItemDefinition* Ammo = Lookup(Definition.AmmoItem);
	return Ammo ? Ammo->WeightKg * Entry.Rounds : 0.0f;
}

float FRubezhInventoryGrid::GetTotalWeight(FRubezhItemLookup Lookup) const
{
	float Total = 0.0f;
	for (const FRubezhInventoryEntry& Entry : Entries)
	{
		if (const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId))
		{
			Total += Definition->WeightKg * Entry.Count;
			if (Entry.Rounds > 0)
			{
				Total += GetLoadedRoundsWeight(Entry, *Definition, Lookup);
			}
		}
	}
	return Total;
}
