// РУБЕЖ: Земля после разлома

#include "Items/RubezhEquipment.h"

namespace Rubezh::Equipment
{
	namespace
	{
		FName GetEquipped(const FEquipped& Equipped, ERubezhEquipSlot Slot)
		{
			const int32 Index = GetSlotIndex(Slot);
			return Equipped.IsValidIndex(Index) ? Equipped[Index] : NAME_None;
		}

		FEquipped Normalized(const FEquipped& Equipped)
		{
			FEquipped Result = Equipped;
			Result.SetNum(SlotCount);
			return Result;
		}

		/** Новый размер сетки после смены надетого; при смене рюкзака сетка перекладывается. */
		bool ResizeFor(FRubezhInventoryGrid& Grid, const FEquipped& Equipped, FRubezhItemLookup Lookup, FIntPoint PocketsSize)
		{
			const FIntPoint NewSize = GetGridSize(Equipped, Lookup, PocketsSize);
			return NewSize == Grid.GetSize() || Grid.Resize(NewSize, Lookup);
		}
	}

	int32 GetSlotIndex(ERubezhEquipSlot Slot)
	{
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			if (AllSlots[Index] == Slot)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	const TCHAR* GetSlotName(ERubezhEquipSlot Slot)
	{
		switch (Slot)
		{
		case ERubezhEquipSlot::Head:     return TEXT("Голова");
		case ERubezhEquipSlot::Armor:    return TEXT("Бронежилет");
		case ERubezhEquipSlot::Top:      return TEXT("Одежда: верх");
		case ERubezhEquipSlot::Bottom:   return TEXT("Одежда: низ");
		case ERubezhEquipSlot::Backpack: return TEXT("Рюкзак");
		default:                         return TEXT("");
		}
	}

	const TCHAR* GetZoneName(ERubezhBodyZone Zone)
	{
		switch (Zone)
		{
		case ERubezhBodyZone::Head:  return TEXT("голова");
		case ERubezhBodyZone::Torso: return TEXT("корпус");
		case ERubezhBodyZone::Arms:  return TEXT("руки");
		case ERubezhBodyZone::Legs:
		default:                     return TEXT("ноги");
		}
	}

	bool Covers(ERubezhEquipSlot Slot, ERubezhBodyZone Zone)
	{
		switch (Slot)
		{
		case ERubezhEquipSlot::Head:   return Zone == ERubezhBodyZone::Head;
		case ERubezhEquipSlot::Armor:  return Zone == ERubezhBodyZone::Torso;
		case ERubezhEquipSlot::Top:    return Zone == ERubezhBodyZone::Torso || Zone == ERubezhBodyZone::Arms;
		case ERubezhEquipSlot::Bottom: return Zone == ERubezhBodyZone::Legs;
		default:                       return false;
		}
	}

	ERubezhBodyZone GetZoneForBone(FName BoneName)
	{
		const FString Name = BoneName.ToString();
		if (Name.Contains(TEXT("head")) || Name.Contains(TEXT("neck")))
		{
			return ERubezhBodyZone::Head;
		}
		if (Name.Contains(TEXT("upperarm")) || Name.Contains(TEXT("lowerarm")) || Name.Contains(TEXT("hand"))
			|| Name.Contains(TEXT("thumb")) || Name.Contains(TEXT("index")) || Name.Contains(TEXT("middle"))
			|| Name.Contains(TEXT("ring")) || Name.Contains(TEXT("pinky")))
		{
			return ERubezhBodyZone::Arms;
		}
		if (Name.Contains(TEXT("thigh")) || Name.Contains(TEXT("calf")) || Name.Contains(TEXT("foot")) || Name.Contains(TEXT("ball")))
		{
			return ERubezhBodyZone::Legs;
		}
		return ERubezhBodyZone::Torso;
	}

	FProtection GetProtection(const FEquipped& Equipped, ERubezhBodyZone Zone, FRubezhItemLookup Lookup)
	{
		FProtection Result;
		for (int32 Index = 0; Index < SlotCount && Index < Equipped.Num(); ++Index)
		{
			const FRubezhItemDefinition* Item = Equipped[Index].IsNone() ? nullptr : Lookup(Equipped[Index]);
			if (Item && Covers(AllSlots[Index], Zone))
			{
				Result.DamageMultiplier *= 1.0f - FMath::Clamp(Item->Protection, 0.0f, 95.0f) * 0.01f;
				Result.BleedChanceMultiplier *= 1.0f - FMath::Clamp(Item->BleedProtection, 0.0f, 100.0f) * 0.01f;
			}
		}
		return Result;
	}

	FIntPoint GetGridSize(const FEquipped& Equipped, FRubezhItemLookup Lookup, FIntPoint PocketsSize)
	{
		const FName Backpack = GetEquipped(Equipped, ERubezhEquipSlot::Backpack);
		const FRubezhItemDefinition* Item = Backpack.IsNone() ? nullptr : Lookup(Backpack);
		return (Item && Item->BackpackWidth > 0 && Item->BackpackHeight > 0) ? FIntPoint(Item->BackpackWidth, Item->BackpackHeight) : PocketsSize;
	}

	float GetCarryBonusKg(const FEquipped& Equipped, FRubezhItemLookup Lookup)
	{
		float Total = 0.0f;
		for (const FName& ItemId : Equipped)
		{
			if (const FRubezhItemDefinition* Item = ItemId.IsNone() ? nullptr : Lookup(ItemId))
			{
				Total += Item->CarryBonusKg;
			}
		}
		return Total;
	}

	float GetWeight(const FEquipped& Equipped, FRubezhItemLookup Lookup)
	{
		float Total = 0.0f;
		for (const FName& ItemId : Equipped)
		{
			if (const FRubezhItemDefinition* Item = ItemId.IsNone() ? nullptr : Lookup(ItemId))
			{
				Total += Item->WeightKg;
			}
		}
		return Total;
	}

	float GetWarmth(const FEquipped& Equipped, FRubezhItemLookup Lookup)
	{
		float Total = 0.0f;
		for (const FName& ItemId : Equipped)
		{
			if (const FRubezhItemDefinition* Item = ItemId.IsNone() ? nullptr : Lookup(ItemId))
			{
				Total += Item->Warmth;
			}
		}
		return Total;
	}

	const TCHAR* Describe(EResult Result)
	{
		switch (Result)
		{
		case EResult::Ok:          return TEXT("");
		case EResult::NotWearable: return TEXT("Это не надевается");
		case EResult::Missing:     return TEXT("Предмета нет");
		case EResult::WontFit:     return TEXT("Вещи не поместятся — сначала освободите место");
		case EResult::NoRoom:
		default:                   return TEXT("Некуда положить снятое — нет места в сетке");
		}
	}

	EResult Equip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, int32 EntryIndex, FRubezhItemLookup Lookup, FIntPoint PocketsSize)
	{
		if (!Grid.GetEntries().IsValidIndex(EntryIndex))
		{
			return EResult::Missing;
		}
		const FRubezhInventoryEntry Entry = Grid.GetEntries()[EntryIndex];
		const FRubezhItemDefinition* Item = Lookup(Entry.ItemId);
		const int32 SlotIndex = Item ? GetSlotIndex(Item->EquipSlot) : INDEX_NONE;
		if (SlotIndex == INDEX_NONE)
		{
			return EResult::NotWearable;
		}

		// Всё делаем на копиях и сохраняем, только если получилось целиком.
		FRubezhInventoryGrid NewGrid = Grid;
		FEquipped NewEquipped = Normalized(Equipped);

		FRubezhInventoryEntry Taken;
		NewGrid.RemoveEntry(EntryIndex, Taken);
		if (Entry.Count > 1 && !NewGrid.AddEntryAt(Entry.ItemId, Entry.Count - 1, Entry.Position, Entry.bRotated, Lookup))
		{
			return EResult::NoRoom;
		}

		const FName Previous = NewEquipped[SlotIndex];
		NewEquipped[SlotIndex] = Entry.ItemId;
		if (!ResizeFor(NewGrid, NewEquipped, Lookup, PocketsSize))
		{
			return EResult::WontFit;
		}

		if (!Previous.IsNone())
		{
			const FRubezhItemDefinition* PreviousItem = Lookup(Previous);
			if (!PreviousItem || NewGrid.AddItem(Previous, *PreviousItem, 1, Lookup) != 0)
			{
				return EResult::NoRoom;
			}
		}

		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		return EResult::Ok;
	}

	EResult TakeOff(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot, FRubezhItemLookup Lookup, FIntPoint PocketsSize, FName& OutItemId)
	{
		const int32 SlotIndex = GetSlotIndex(Slot);
		FEquipped NewEquipped = Normalized(Equipped);
		if (SlotIndex == INDEX_NONE || NewEquipped[SlotIndex].IsNone())
		{
			return EResult::Missing;
		}

		FRubezhInventoryGrid NewGrid = Grid;
		const FName ItemId = NewEquipped[SlotIndex];
		NewEquipped[SlotIndex] = NAME_None;
		if (!ResizeFor(NewGrid, NewEquipped, Lookup, PocketsSize))
		{
			return EResult::WontFit;
		}

		OutItemId = ItemId;
		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		return EResult::Ok;
	}

	EResult Unequip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot,
		TOptional<FIntPoint> Position, bool bRotated, FRubezhItemLookup Lookup, FIntPoint PocketsSize)
	{
		FRubezhInventoryGrid NewGrid = Grid;
		FEquipped NewEquipped = Equipped;
		FName ItemId;
		const EResult Result = TakeOff(NewGrid, NewEquipped, Slot, Lookup, PocketsSize, ItemId);
		if (Result != EResult::Ok)
		{
			return Result;
		}

		const FRubezhItemDefinition* Item = Lookup(ItemId);
		const bool bPlaced = Item && (Position.IsSet()
			? NewGrid.AddEntryAt(ItemId, 1, Position.GetValue(), bRotated, Lookup)
			: NewGrid.AddItem(ItemId, *Item, 1, Lookup) == 0);
		if (!bPlaced)
		{
			return EResult::NoRoom;
		}

		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		return EResult::Ok;
	}

	FString DescribeStats(const FRubezhItemDefinition& Item)
	{
		if (Item.EquipSlot == ERubezhEquipSlot::None)
		{
			return FString();
		}
		if (Item.EquipSlot == ERubezhEquipSlot::Backpack)
		{
			return FString::Printf(TEXT("Рюкзак: %d×%d клеток, +%.0f кг к лимиту веса"), Item.BackpackWidth, Item.BackpackHeight, Item.CarryBonusKg);
		}

		TArray<FString> Zones;
		for (const ERubezhBodyZone Zone : { ERubezhBodyZone::Head, ERubezhBodyZone::Torso, ERubezhBodyZone::Arms, ERubezhBodyZone::Legs })
		{
			if (Covers(Item.EquipSlot, Zone))
			{
				Zones.Add(GetZoneName(Zone));
			}
		}

		FString Stats = FString::Printf(TEXT("%s: защита %.0f %%, кровотечение −%.0f %%"), *FString::Join(Zones, TEXT(", ")), Item.Protection, Item.BleedProtection);
		if (Item.Warmth > 0.0f)
		{
			Stats += FString::Printf(TEXT(", тепло %.0f"), Item.Warmth);
		}
		return Stats;
	}
}
