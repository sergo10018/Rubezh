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

		/** Новое число ячеек после смены надетого; при смене рюкзака вещи перекладываются. */
		bool ResizeFor(FRubezhInventoryGrid& Grid, const FEquipped& Equipped, FRubezhItemLookup Lookup, int32 PocketSlots)
		{
			const int32 NewCount = GetSlotCount(Equipped, Lookup, PocketSlots);
			return NewCount == Grid.GetSlotCount() || Grid.Resize(NewCount);
		}

		float GetDurability(const FEquippedWear* Wear, int32 Index)
		{
			return (Wear && Wear->IsValidIndex(Index)) ? (*Wear)[Index] : MaxGearDurability;
		}

		FEquippedWear NormalizedWear(const FEquippedWear* Wear)
		{
			FEquippedWear Result;
			Result.Init(MaxGearDurability, SlotCount);
			for (int32 Index = 0; Wear && Index < SlotCount && Index < Wear->Num(); ++Index)
			{
				Result[Index] = (*Wear)[Index];
			}
			return Result;
		}

		FRubezhInventoryEntry MakeGearEntry(FName ItemId, float Durability)
		{
			FRubezhInventoryEntry Entry;
			Entry.ItemId = ItemId;
			Entry.Count = 1;
			Entry.Durability = Durability;
			return Entry;
		}
	}

	bool CanWear(const FRubezhItemDefinition& Item)
	{
		return Item.EquipSlot != ERubezhEquipSlot::None && (Item.Protection > 0.0f || Item.BleedProtection > 0.0f);
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

	FProtection GetProtection(const FEquipped& Equipped, ERubezhBodyZone Zone, FRubezhItemLookup Lookup, const FEquippedWear* Wear)
	{
		FProtection Result;
		for (int32 Index = 0; Index < SlotCount && Index < Equipped.Num(); ++Index)
		{
			const FRubezhItemDefinition* Item = Equipped[Index].IsNone() ? nullptr : Lookup(Equipped[Index]);
			if (Item && Covers(AllSlots[Index], Zone))
			{
				const float Condition = FMath::Clamp(GetDurability(Wear, Index), 0.0f, MaxGearDurability) / MaxGearDurability;
				Result.DamageMultiplier *= 1.0f - FMath::Clamp(Item->Protection, 0.0f, 95.0f) * 0.01f * Condition;
				Result.BleedChanceMultiplier *= 1.0f - FMath::Clamp(Item->BleedProtection, 0.0f, 100.0f) * 0.01f * Condition;
			}
		}
		return Result;
	}

	void ApplyHitWear(const FEquipped& Equipped, FEquippedWear& Wear, ERubezhBodyZone Zone, float RawDamage, FRubezhItemLookup Lookup)
	{
		Wear = NormalizedWear(&Wear);
		for (int32 Index = 0; Index < SlotCount && Index < Equipped.Num(); ++Index)
		{
			const FRubezhItemDefinition* Item = Equipped[Index].IsNone() ? nullptr : Lookup(Equipped[Index]);
			if (Item && CanWear(*Item) && Covers(AllSlots[Index], Zone))
			{
				Wear[Index] = FMath::Max(Wear[Index] - FMath::Max(RawDamage, 0.0f) * WearPerDamagePoint, 0.0f);
			}
		}
	}

	int32 GetSlotCount(const FEquipped& Equipped, FRubezhItemLookup Lookup, int32 PocketSlots)
	{
		const FName Backpack = GetEquipped(Equipped, ERubezhEquipSlot::Backpack);
		const FRubezhItemDefinition* Item = Backpack.IsNone() ? nullptr : Lookup(Backpack);
		return (Item && Item->BackpackSlots > 0) ? Item->BackpackSlots : PocketSlots;
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
		case EResult::SlotTaken:   return TEXT("Сначала снимите свой рюкзак");
		case EResult::NoRoom:
		default:                   return TEXT("Некуда положить — нет свободной ячейки");
		}
	}

	EResult Equip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, int32 EntryIndex, FRubezhItemLookup Lookup, int32 PocketSlots, FEquippedWear* Wear)
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
		FEquippedWear NewWear = NormalizedWear(Wear);

		NewGrid.ReduceEntry(EntryIndex, 1);

		const FName Previous = NewEquipped[SlotIndex];
		NewEquipped[SlotIndex] = Entry.ItemId;
		if (!ResizeFor(NewGrid, NewEquipped, Lookup, PocketSlots))
		{
			return EResult::WontFit;
		}

		if (!Previous.IsNone())
		{
			const FRubezhItemDefinition* PreviousItem = Lookup(Previous);
			if (!PreviousItem || NewGrid.AddEntry(MakeGearEntry(Previous, NewWear[SlotIndex]), *PreviousItem) != 0)
			{
				return EResult::NoRoom;
			}
		}

		NewWear[SlotIndex] = Entry.Durability;
		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		if (Wear)
		{
			*Wear = MoveTemp(NewWear);
		}
		return EResult::Ok;
	}

	EResult TakeOff(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot, FRubezhItemLookup Lookup, int32 PocketSlots, FName& OutItemId)
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
		if (!ResizeFor(NewGrid, NewEquipped, Lookup, PocketSlots))
		{
			return EResult::WontFit;
		}

		OutItemId = ItemId;
		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		return EResult::Ok;
	}

	EResult Unequip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot,
		TOptional<int32> TargetSlot, FRubezhItemLookup Lookup, int32 PocketSlots, FEquippedWear* Wear)
	{
		FRubezhInventoryGrid NewGrid = Grid;
		FEquipped NewEquipped = Equipped;
		FName ItemId;
		const EResult Result = TakeOff(NewGrid, NewEquipped, Slot, Lookup, PocketSlots, ItemId);
		if (Result != EResult::Ok)
		{
			return Result;
		}

		const FRubezhItemDefinition* Item = Lookup(ItemId);
		const int32 SlotIndex = GetSlotIndex(Slot);
		const float Durability = GetDurability(Wear, SlotIndex);
		bool bPlaced = false;
		if (Item && TargetSlot.IsSet())
		{
			bPlaced = NewGrid.AddEntryAt(ItemId, 1, TargetSlot.GetValue(), Lookup);
			if (FRubezhInventoryEntry* Placed = bPlaced ? NewGrid.GetEntryMutable(NewGrid.FindEntryAt(TargetSlot.GetValue())) : nullptr)
			{
				Placed->Durability = Durability;
			}
		}
		else if (Item)
		{
			bPlaced = NewGrid.AddEntry(MakeGearEntry(ItemId, Durability), *Item) == 0;
		}
		if (!bPlaced)
		{
			return EResult::NoRoom;
		}

		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		if (Wear)
		{
			*Wear = NormalizedWear(Wear);
			(*Wear)[SlotIndex] = MaxGearDurability;
		}
		return EResult::Ok;
	}

	EResult DropBag(FRubezhInventoryGrid& Grid, FEquipped& Equipped, int32 PocketSlots, FName& OutBagId, FRubezhInventoryGrid& OutContents)
	{
		const int32 SlotIndex = GetSlotIndex(ERubezhEquipSlot::Backpack);
		FEquipped NewEquipped = Normalized(Equipped);
		if (NewEquipped[SlotIndex].IsNone())
		{
			return EResult::Missing;
		}

		OutBagId = NewEquipped[SlotIndex];
		OutContents = Grid;
		NewEquipped[SlotIndex] = NAME_None;
		Equipped = MoveTemp(NewEquipped);
		Grid = FRubezhInventoryGrid(PocketSlots);
		return EResult::Ok;
	}

	EResult WearBag(FRubezhInventoryGrid& Grid, FEquipped& Equipped, FName BagId, const FRubezhInventoryGrid& BagContents, FRubezhItemLookup Lookup)
	{
		const int32 SlotIndex = GetSlotIndex(ERubezhEquipSlot::Backpack);
		const FRubezhItemDefinition* Bag = Lookup(BagId);
		FEquipped NewEquipped = Normalized(Equipped);
		if (!Bag || Bag->EquipSlot != ERubezhEquipSlot::Backpack)
		{
			return EResult::NotWearable;
		}
		if (!NewEquipped[SlotIndex].IsNone())
		{
			return EResult::SlotTaken;
		}

		// Брошенный рюкзак со своим содержимым, плюс то, что было в карманах.
		NewEquipped[SlotIndex] = BagId;
		FRubezhInventoryGrid NewGrid = BagContents;
		if (!NewGrid.Resize(GetSlotCount(NewEquipped, Lookup, Grid.GetSlotCount())))
		{
			return EResult::WontFit;
		}
		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			const FRubezhItemDefinition* Item = Lookup(Entry.ItemId);
			if (!Item || NewGrid.AddEntry(Entry, *Item) != 0)
			{
				return EResult::WontFit;
			}
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
			return FString::Printf(TEXT("Рюкзак: %d ячеек, +%.0f кг к лимиту веса"), Item.BackpackSlots, Item.CarryBonusKg);
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
