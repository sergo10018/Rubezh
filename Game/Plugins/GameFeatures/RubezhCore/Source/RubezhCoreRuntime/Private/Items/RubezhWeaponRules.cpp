// РУБЕЖ: Земля после разлома

#include "Items/RubezhWeaponRules.h"

namespace Rubezh::Weapons
{
	namespace
	{
		constexpr float CentimetersPerMeter = 100.0f;
		constexpr float FalloffEndsAtRangeMultiple = 2.0f;

		FWeaponSlots Normalized(const FWeaponSlots& Slots)
		{
			FWeaponSlots Result = Slots;
			Result.SetNum(SlotCount);
			return Result;
		}

		TOptional<int32> PickSlot(const FWeaponSlots& Slots, ERubezhWeaponClass WeaponClass)
		{
			TOptional<int32> FirstAccepting;
			for (int32 Index = 0; Index < SlotCount; ++Index)
			{
				if (!SlotAccepts(WeaponClass, Index))
				{
					continue;
				}
				if (Slots[Index].ItemId.IsNone())
				{
					return Index;
				}
				if (!FirstAccepting.IsSet())
				{
					FirstAccepting = Index;
				}
			}
			return FirstAccepting;
		}
	}

	bool SlotAccepts(ERubezhWeaponClass WeaponClass, int32 WeaponSlot)
	{
		if (WeaponSlot < 0 || WeaponSlot >= SlotCount)
		{
			return false;
		}
		return (WeaponSlot == SidearmSlot) == (WeaponClass == ERubezhWeaponClass::Sidearm);
	}

	const TCHAR* GetSlotName(int32 WeaponSlot)
	{
		switch (WeaponSlot)
		{
		case 0:				return TEXT("Основное оружие 1");
		case 1:				return TEXT("Основное оружие 2");
		case SidearmSlot:	return TEXT("Пистолет");
		default:			return TEXT("?");
		}
	}

	const TCHAR* Describe(EResult Result)
	{
		switch (Result)
		{
		case EResult::Ok:			return TEXT("готово");
		case EResult::NotWeapon:	return TEXT("Это не оружие");
		case EResult::WrongSlot:	return TEXT("Это оружие не берётся в этот слот");
		case EResult::Missing:		return TEXT("Здесь ничего нет");
		case EResult::NoRoom:		return TEXT("В рюкзаке нет свободной ячейки");
		default:					return TEXT("?");
		}
	}

	EResult Equip(FRubezhInventoryGrid& Grid, FWeaponSlots& Slots, int32 EntryIndex, TOptional<int32> WeaponSlot, FWeaponLookup Weapons)
	{
		if (!Grid.GetEntries().IsValidIndex(EntryIndex))
		{
			return EResult::Missing;
		}
		const FRubezhInventoryEntry Entry = Grid.GetEntries()[EntryIndex];
		const FRubezhWeaponDefinition* Weapon = Weapons(Entry.ItemId);
		if (!Weapon)
		{
			return EResult::NotWeapon;
		}

		FWeaponSlots NewSlots = Normalized(Slots);
		const TOptional<int32> Target = WeaponSlot.IsSet() ? WeaponSlot : PickSlot(NewSlots, Weapon->WeaponClass);
		if (!Target.IsSet() || !SlotAccepts(Weapon->WeaponClass, Target.GetValue()))
		{
			return EResult::WrongSlot;
		}

		FRubezhInventoryGrid NewGrid = Grid;
		FRubezhInventoryEntry Removed;
		NewGrid.RemoveEntry(EntryIndex, Removed);

		// Что было в слоте — в освободившуюся ячейку: место всегда есть.
		const FRubezhInventoryEntry Previous = NewSlots[Target.GetValue()];
		if (!Previous.ItemId.IsNone())
		{
			NewGrid.PlaceEntry(Previous, Entry.Slot);
		}
		NewSlots[Target.GetValue()] = Entry;

		Grid = MoveTemp(NewGrid);
		Slots = MoveTemp(NewSlots);
		return EResult::Ok;
	}

	EResult Unequip(FRubezhInventoryGrid& Grid, FWeaponSlots& Slots, int32 WeaponSlot, TOptional<int32> Cell)
	{
		if (!Slots.IsValidIndex(WeaponSlot) || Slots[WeaponSlot].ItemId.IsNone())
		{
			return EResult::Missing;
		}

		const int32 Target = (Cell.IsSet() && Grid.IsSlotFree(Cell.GetValue())) ? Cell.GetValue() : Grid.FindFreeSlot();
		if (!Grid.PlaceEntry(Slots[WeaponSlot], Target))
		{
			return EResult::NoRoom;
		}
		Slots[WeaponSlot] = FRubezhInventoryEntry();
		return EResult::Ok;
	}

	int32 GetCapacity(const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup)
	{
		if (Weapon.MagazineItem.IsNone())
		{
			return Weapon.TubeCapacity;
		}
		const FRubezhItemDefinition* Magazine = Lookup(Weapon.MagazineItem);
		return Magazine ? Magazine->MagazineCapacity : 0;
	}

	int32 CountSpareRounds(const FRubezhInventoryGrid& Grid, const FRubezhWeaponDefinition& Weapon)
	{
		if (Weapon.MagazineItem.IsNone())
		{
			return Grid.CountItem(Weapon.AmmoItem);
		}

		int32 Total = 0;
		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			Total += Entry.ItemId == Weapon.MagazineItem ? Entry.Rounds : 0;
		}
		return Total;
	}

	bool Reload(FRubezhInventoryGrid& Grid, FRubezhInventoryEntry& WeaponEntry, const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup)
	{
		if (Weapon.MagazineItem.IsNone())
		{
			const int32 Missing = FMath::Max(Weapon.TubeCapacity - WeaponEntry.Rounds, 0);
			const int32 Taken = Grid.RemoveItem(Weapon.AmmoItem, Missing);
			WeaponEntry.Rounds += Taken;
			return Taken > 0;
		}

		int32 BestIndex = INDEX_NONE;
		int32 BestRounds = 0;
		const TArray<FRubezhInventoryEntry>& Entries = Grid.GetEntries();
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			if (Entries[Index].ItemId == Weapon.MagazineItem && Entries[Index].Rounds > BestRounds)
			{
				BestIndex = Index;
				BestRounds = Entries[Index].Rounds;
			}
		}
		if (BestIndex == INDEX_NONE)
		{
			return false;
		}

		if (WeaponEntry.bHasMagazine)
		{
			// Магазины меняются местами: снятый ложится в ячейку поставленного.
			Grid.GetEntryMutable(BestIndex)->Rounds = WeaponEntry.Rounds;
		}
		else
		{
			Grid.ReduceEntry(BestIndex, 1);
		}
		WeaponEntry.Rounds = BestRounds;
		WeaponEntry.bHasMagazine = true;
		return true;
	}

	bool CanLoadRound(const FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup)
	{
		if (!Grid.GetEntries().IsValidIndex(MagazineIndex))
		{
			return false;
		}
		const FRubezhInventoryEntry& Entry = Grid.GetEntries()[MagazineIndex];
		const FRubezhItemDefinition* Magazine = Lookup(Entry.ItemId);
		return Magazine && Magazine->MagazineCapacity > Entry.Rounds && Grid.CountItem(Magazine->AmmoItem) > 0;
	}

	bool LoadRound(FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup)
	{
		if (!CanLoadRound(Grid, MagazineIndex, Lookup))
		{
			return false;
		}

		// Ищем стопку заново: убранная пустая стопка сдвигает индексы, поэтому магазин запоминаем по ячейке.
		const int32 MagazineCell = Grid.GetEntries()[MagazineIndex].Slot;
		Grid.RemoveItem(Lookup(Grid.GetEntries()[MagazineIndex].ItemId)->AmmoItem, 1);
		Grid.GetEntryMutable(Grid.FindEntryAt(MagazineCell))->Rounds += 1;
		return true;
	}

	int32 UnloadMagazine(FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup)
	{
		if (!Grid.GetEntries().IsValidIndex(MagazineIndex))
		{
			return 0;
		}
		const FRubezhInventoryEntry Entry = Grid.GetEntries()[MagazineIndex];
		const FRubezhItemDefinition* Magazine = Lookup(Entry.ItemId);
		const FRubezhItemDefinition* Ammo = Magazine ? Lookup(Magazine->AmmoItem) : nullptr;
		if (!Ammo || Entry.Rounds <= 0)
		{
			return 0;
		}

		const int32 NotAdded = Grid.AddItem(Magazine->AmmoItem, *Ammo, Entry.Rounds);
		Grid.GetEntryMutable(Grid.FindEntryAt(Entry.Slot))->Rounds = NotAdded;
		return Entry.Rounds - NotAdded;
	}

	float GetMisfireChance(float Durability)
	{
		if (Durability >= MisfireStartsBelow)
		{
			return 0.0f;
		}
		if (Durability >= MisfireSevereBelow)
		{
			return FMath::GetMappedRangeValueClamped(FVector2f(MisfireStartsBelow, MisfireSevereBelow), FVector2f(0.0f, MisfireChanceAtSevere), Durability);
		}
		return FMath::GetMappedRangeValueClamped(FVector2f(MisfireSevereBelow, 0.0f), FVector2f(MisfireChanceAtSevere, MisfireChanceAtZero), Durability);
	}

	float ApplyWear(float Durability)
	{
		return FMath::Max(Durability - WearPerShot, 0.0f);
	}

	float GetZoneDamageMultiplier(ERubezhBodyZone Zone)
	{
		switch (Zone)
		{
		case ERubezhBodyZone::Head:		return HeadDamageMultiplier;
		case ERubezhBodyZone::Arms:
		case ERubezhBodyZone::Legs:		return LimbDamageMultiplier;
		case ERubezhBodyZone::Torso:
		default:						return TorsoDamageMultiplier;
		}
	}

	float GetDistanceMultiplier(float DistanceCm, float EffectiveRangeM)
	{
		const float Range = FMath::Max(EffectiveRangeM, 1.0f) * CentimetersPerMeter;
		return FMath::GetMappedRangeValueClamped(FVector2f(Range, Range * FalloffEndsAtRangeMultiple), FVector2f(1.0f, MinDistanceMultiplier), DistanceCm);
	}

	FRecoil ComputeRecoil(const FRubezhWeaponDefinition& Weapon, int32 ShotsInBurst, bool bCrouched, float YawRoll)
	{
		const float Growth = 1.0f + Weapon.RecoilGrowth * FMath::Clamp(ShotsInBurst, 0, MaxRecoilGrowthShots);
		const float Stance = bCrouched ? CrouchRecoilMultiplier : 1.0f;

		FRecoil Recoil;
		Recoil.Pitch = Weapon.RecoilPitch * Growth * Stance;
		Recoil.Yaw = Weapon.RecoilYaw * Growth * Stance * FMath::Clamp(YawRoll, -1.0f, 1.0f);
		return Recoil;
	}

	FRubezhInventoryEntry MakeLoaded(FName ItemId, const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup)
	{
		FRubezhInventoryEntry Entry;
		Entry.ItemId = ItemId;
		Entry.Count = 1;
		Entry.Rounds = GetCapacity(Weapon, Lookup);
		Entry.bHasMagazine = !Weapon.MagazineItem.IsNone();
		return Entry;
	}
}
