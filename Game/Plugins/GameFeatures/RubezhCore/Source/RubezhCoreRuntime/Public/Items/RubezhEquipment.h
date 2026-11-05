// РУБЕЖ: Земля после разлома
// Правила снаряжения: надеть/снять, защита по частям тела, рюкзак. Только данные и математика — проверяется тестами.
// Характеристики предметов — в data/items.csv (EquipSlot, Protection, BleedProtection, Warmth, BackpackWidth, BackpackHeight, CarryBonusKg).

#pragma once

#include "Items/RubezhEquipSlot.h"
#include "Items/RubezhInventoryGrid.h"

namespace Rubezh::Equipment
{
	/** Сколько слотов снаряжения (Head … Backpack). Надетое хранится массивом такой длины. */
	inline constexpr int32 SlotCount = 5;

	/** Порядок слотов в окне и в массиве надетого. */
	inline constexpr ERubezhEquipSlot AllSlots[SlotCount] = {
		ERubezhEquipSlot::Head, ERubezhEquipSlot::Armor, ERubezhEquipSlot::Top, ERubezhEquipSlot::Bottom, ERubezhEquipSlot::Backpack };

	/** Индекс слота в массиве надетого; INDEX_NONE для None. */
	RUBEZHCORERUNTIME_API int32 GetSlotIndex(ERubezhEquipSlot Slot);

	RUBEZHCORERUNTIME_API const TCHAR* GetSlotName(ERubezhEquipSlot Slot);
	RUBEZHCORERUNTIME_API const TCHAR* GetZoneName(ERubezhBodyZone Zone);

	/** Закрывает ли предмет из этого слота часть тела. Бронежилет — корпус, верх одежды — корпус и руки, низ — ноги. */
	RUBEZHCORERUNTIME_API bool Covers(ERubezhEquipSlot Slot, ERubezhBodyZone Zone);

	/** Часть тела по кости манекена. Неизвестная кость или попадание без кости (взрыв) — корпус. */
	RUBEZHCORERUNTIME_API ERubezhBodyZone GetZoneForBone(FName BoneName);

	/** Во сколько раз уменьшаются урон и шанс кровотечения в части тела (слои защиты перемножаются). */
	struct FProtection
	{
		float DamageMultiplier = 1.0f;
		float BleedChanceMultiplier = 1.0f;
	};

	/** Надетое: массив длиной SlotCount, None — слот пуст. */
	using FEquipped = TArray<FName>;

	RUBEZHCORERUNTIME_API FProtection GetProtection(const FEquipped& Equipped, ERubezhBodyZone Zone, FRubezhItemLookup Lookup);

	/** Размер сетки: рюкзака или, если его нет, карманов. */
	RUBEZHCORERUNTIME_API FIntPoint GetGridSize(const FEquipped& Equipped, FRubezhItemLookup Lookup, FIntPoint PocketsSize);

	RUBEZHCORERUNTIME_API float GetCarryBonusKg(const FEquipped& Equipped, FRubezhItemLookup Lookup);
	RUBEZHCORERUNTIME_API float GetWeight(const FEquipped& Equipped, FRubezhItemLookup Lookup);
	RUBEZHCORERUNTIME_API float GetWarmth(const FEquipped& Equipped, FRubezhItemLookup Lookup);

	/** Чем закончилась попытка надеть или снять. */
	enum class EResult : uint8
	{
		Ok,
		NotWearable,	// предмет не надевается
		Missing,		// нет такого предмета или слот пуст
		WontFit,		// вещи не влезут в новый рюкзак (или в карманы)
		NoRoom			// снятому предмету нет места в сетке
	};

	RUBEZHCORERUNTIME_API const TCHAR* Describe(EResult Result);

	/**
	 * Надеть предмет из сетки. Что было в слоте — отправляется в сетку. Если меняется рюкзак, сетка меняет размер.
	 * При неудаче Grid и Equipped не меняются.
	 */
	RUBEZHCORERUNTIME_API EResult Equip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, int32 EntryIndex, FRubezhItemLookup Lookup, FIntPoint PocketsSize);

	/** Снять в сетку: на указанное место (Position) или куда влезет (Position не задан). */
	RUBEZHCORERUNTIME_API EResult Unequip(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot,
		TOptional<FIntPoint> Position, bool bRotated, FRubezhItemLookup Lookup, FIntPoint PocketsSize);

	/** Снять, никуда не кладя (чтобы бросить на землю). OutItemId — что сняли. */
	RUBEZHCORERUNTIME_API EResult TakeOff(FRubezhInventoryGrid& Grid, FEquipped& Equipped, ERubezhEquipSlot Slot,
		FRubezhItemLookup Lookup, FIntPoint PocketsSize, FName& OutItemId);

	/** Характеристики предмета одной-двумя строками для подсказки. Пусто — не снаряжение. */
	RUBEZHCORERUNTIME_API FString DescribeStats(const FRubezhItemDefinition& Item);
}
