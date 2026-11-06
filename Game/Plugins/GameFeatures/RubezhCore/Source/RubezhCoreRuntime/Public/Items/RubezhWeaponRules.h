// РУБЕЖ: Земля после разлома
// Правила оружия: слоты, магазины, перезарядка, износ, осечки, отдача, урон по частям тела и дистанции
// (docs/gdd/01_Системы_прототипа.md §1). Только данные и математика — проверяется тестами.

#pragma once

#include "Items/RubezhEquipSlot.h"
#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhWeaponDefinition.h"
#include "Templates/Function.h"

namespace Rubezh::Weapons
{
	/** Слоты: 0 и 1 — основное оружие, 2 — пистолет. Совпадают со слотами быстрого доступа Lyra (клавиши 1, 2, 3). */
	inline constexpr int32 SlotCount = 3;
	inline constexpr int32 SidearmSlot = 2;

	// Износ и осечки.
	inline constexpr float MaxDurability = 100.0f;
	inline constexpr float WearPerShot = 0.05f;
	inline constexpr float MisfireStartsBelow = 50.0f;
	inline constexpr float MisfireSevereBelow = 20.0f;
	inline constexpr float MisfireChanceAtSevere = 0.03f;
	inline constexpr float MisfireChanceAtZero = 0.10f;

	// Части тела.
	inline constexpr float HeadDamageMultiplier = 2.5f;
	inline constexpr float TorsoDamageMultiplier = 1.0f;
	inline constexpr float LimbDamageMultiplier = 0.7f;

	/** На двойной эффективной дальности урон падает до этой доли и дальше не меняется. */
	inline constexpr float MinDistanceMultiplier = 0.5f;

	// Отдача.
	inline constexpr float CrouchRecoilMultiplier = 0.7f;
	inline constexpr int32 MaxRecoilGrowthShots = 10;
	/** Если пауза между выстрелами больше — считаем, что началась новая очередь. */
	inline constexpr float BurstResetSeconds = 0.4f;

	/** Сколько секунд набивается один патрон в магазин. */
	inline constexpr float SecondsPerLoadedRound = 0.2f;

	using FWeaponLookup = TFunctionRef<const FRubezhWeaponDefinition*(FName)>;

	/** Оружие в слотах: массив длиной SlotCount, пустой ItemId — слот свободен. */
	using FWeaponSlots = TArray<FRubezhInventoryEntry>;

	RUBEZHCORERUNTIME_API bool SlotAccepts(ERubezhWeaponClass WeaponClass, int32 WeaponSlot);
	RUBEZHCORERUNTIME_API const TCHAR* GetSlotName(int32 WeaponSlot);

	enum class EResult : uint8
	{
		Ok,
		NotWeapon,		// предмет — не оружие
		WrongSlot,		// этот предмет не берётся в этот слот
		Missing,		// нет такой стопки или слот пуст
		NoRoom			// снятому оружию нет свободной ячейки
	};

	RUBEZHCORERUNTIME_API const TCHAR* Describe(EResult Result);

	/**
	 * Взять оружие из рюкзака в слот (не задан — первый подходящий свободный, иначе первый подходящий).
	 * Что было в слоте, ложится в освободившуюся ячейку. При неудаче ничего не меняется.
	 */
	RUBEZHCORERUNTIME_API EResult Equip(FRubezhInventoryGrid& Grid, FWeaponSlots& Slots, int32 EntryIndex, TOptional<int32> WeaponSlot, FWeaponLookup Weapons);

	/** Убрать оружие из слота в рюкзак: в ячейку Cell или (не задана) в первую свободную. */
	RUBEZHCORERUNTIME_API EResult Unequip(FRubezhInventoryGrid& Grid, FWeaponSlots& Slots, int32 WeaponSlot, TOptional<int32> Cell);

	/** Сколько патронов вмещает оружие: магазин или само оружие. */
	RUBEZHCORERUNTIME_API int32 GetCapacity(const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup);

	/** Сколько патронов можно зарядить из рюкзака: в подходящих магазинах или россыпью для оружия без магазина. */
	RUBEZHCORERUNTIME_API int32 CountSpareRounds(const FRubezhInventoryGrid& Grid, const FRubezhWeaponDefinition& Weapon);

	/**
	 * Перезарядить. С магазином — ставится самый полный из рюкзака, снятый со своими патронами ложится в ту же ячейку.
	 * Без магазина — патроны докладываются из рюкзака. false — зарядить нечем.
	 */
	RUBEZHCORERUNTIME_API bool Reload(FRubezhInventoryGrid& Grid, FRubezhInventoryEntry& WeaponEntry, const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup);

	/** Подходят ли патроны к магазину, и есть ли в нём место. */
	RUBEZHCORERUNTIME_API bool CanLoadRound(const FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup);

	/** Переложить один патрон из рюкзака в магазин. false — магазин полон или патронов нет. */
	RUBEZHCORERUNTIME_API bool LoadRound(FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup);

	/** Разрядить магазин в рюкзак, сколько влезет. Возвращает, сколько патронов вынуто. */
	RUBEZHCORERUNTIME_API int32 UnloadMagazine(FRubezhInventoryGrid& Grid, int32 MagazineIndex, FRubezhItemLookup Lookup);

	/** Шанс осечки (0–1) при такой прочности. */
	RUBEZHCORERUNTIME_API float GetMisfireChance(float Durability);

	/** Прочность после выстрела. */
	RUBEZHCORERUNTIME_API float ApplyWear(float Durability);

	RUBEZHCORERUNTIME_API float GetZoneDamageMultiplier(ERubezhBodyZone Zone);

	/** Доля урона на дистанции: полный в пределах дальности, затем линейно до половины на двойной дальности. */
	RUBEZHCORERUNTIME_API float GetDistanceMultiplier(float DistanceCm, float EffectiveRangeM);

	/** На сколько градусов выстрел сдвигает прицел. */
	struct FRecoil
	{
		float Pitch = 0.0f;
		float Yaw = 0.0f;
	};

	/** ShotsInBurst — сколько выстрелов этой очереди уже было; YawRoll — случайное число от -1 до 1. */
	RUBEZHCORERUNTIME_API FRecoil ComputeRecoil(const FRubezhWeaponDefinition& Weapon, int32 ShotsInBurst, bool bCrouched, float YawRoll);

	/** Новое оружие из рюкзака или ящика: без магазина и патронов, полная прочность. */
	RUBEZHCORERUNTIME_API FRubezhInventoryEntry MakeLoaded(FName ItemId, const FRubezhWeaponDefinition& Weapon, FRubezhItemLookup Lookup);
}
