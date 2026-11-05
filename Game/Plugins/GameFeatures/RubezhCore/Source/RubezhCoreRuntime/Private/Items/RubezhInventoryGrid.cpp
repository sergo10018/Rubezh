// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryGrid.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryGrid)

namespace
{
	bool RectsOverlap(FIntPoint PosA, FIntPoint SizeA, FIntPoint PosB, FIntPoint SizeB)
	{
		return PosA.X < PosB.X + SizeB.X && PosB.X < PosA.X + SizeA.X
			&& PosA.Y < PosB.Y + SizeB.Y && PosB.Y < PosA.Y + SizeA.Y;
	}
}

FIntPoint FRubezhInventoryGrid::GetEntryFootprint(const FRubezhInventoryEntry& Entry, FRubezhItemLookup Lookup) const
{
	const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
	// Неизвестный предмет (удалён из таблицы) всё равно занимает клетку, чтобы не потерять его при сохранении.
	return Definition ? Definition->GetFootprint(Entry.bRotated) : FIntPoint(1, 1);
}

bool FRubezhInventoryGrid::IsAreaFree(FIntPoint Position, FIntPoint Footprint, FRubezhItemLookup Lookup, int32 IgnoreIndex) const
{
	const bool bInside = Position.X >= 0 && Position.Y >= 0
		&& Position.X + Footprint.X <= Size.X && Position.Y + Footprint.Y <= Size.Y;
	if (!bInside)
	{
		return false;
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Index != IgnoreIndex && RectsOverlap(Position, Footprint, Entries[Index].Position, GetEntryFootprint(Entries[Index], Lookup)))
		{
			return false;
		}
	}
	return true;
}

bool FRubezhInventoryGrid::FindFreeSpot(const FRubezhItemDefinition& Definition, FRubezhItemLookup Lookup, FIntPoint& OutPosition, bool& bOutRotated) const
{
	const bool bSquare = Definition.Width == Definition.Height;
	for (const bool bRotated : { false, true })
	{
		if (bRotated && bSquare)
		{
			break;
		}

		const FIntPoint Footprint = Definition.GetFootprint(bRotated);
		for (int32 Y = 0; Y + Footprint.Y <= Size.Y; ++Y)
		{
			for (int32 X = 0; X + Footprint.X <= Size.X; ++X)
			{
				if (IsAreaFree(FIntPoint(X, Y), Footprint, Lookup))
				{
					OutPosition = FIntPoint(X, Y);
					bOutRotated = bRotated;
					return true;
				}
			}
		}
	}
	return false;
}

int32 FRubezhInventoryGrid::AddItem(FName ItemId, const FRubezhItemDefinition& Definition, int32 Count, FRubezhItemLookup Lookup)
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
		FIntPoint Position;
		bool bRotated = false;
		if (!FindFreeSpot(Definition, Lookup, Position, bRotated))
		{
			break;
		}

		FRubezhInventoryEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.ItemId = ItemId;
		Entry.Count = FMath::Min(MaxStack, Remaining);
		Entry.Position = Position;
		Entry.bRotated = bRotated;
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

bool FRubezhInventoryGrid::MoveEntry(int32 EntryIndex, FIntPoint NewPosition, bool bRotated, FRubezhItemLookup Lookup)
{
	if (!Entries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	FRubezhInventoryEntry& Entry = Entries[EntryIndex];
	const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId);
	const FIntPoint Footprint = Definition ? Definition->GetFootprint(bRotated) : FIntPoint(1, 1);
	if (!IsAreaFree(NewPosition, Footprint, Lookup, EntryIndex))
	{
		return false;
	}

	Entry.Position = NewPosition;
	Entry.bRotated = bRotated;
	return true;
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

float FRubezhInventoryGrid::GetTotalWeight(FRubezhItemLookup Lookup) const
{
	float Total = 0.0f;
	for (const FRubezhInventoryEntry& Entry : Entries)
	{
		if (const FRubezhItemDefinition* Definition = Lookup(Entry.ItemId))
		{
			Total += Definition->WeightKg * Entry.Count;
		}
	}
	return Total;
}
