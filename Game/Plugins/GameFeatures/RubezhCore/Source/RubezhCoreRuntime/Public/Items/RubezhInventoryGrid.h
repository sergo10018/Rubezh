// РУБЕЖ: Земля после разлома

#pragma once

#include "Items/RubezhItemDefinition.h"
#include "Templates/Function.h"

#include "RubezhInventoryGrid.generated.h"

/** Поиск описания предмета по идентификатору (обычно — строка DataTable). nullptr — предмет неизвестен. */
using FRubezhItemLookup = TFunctionRef<const FRubezhItemDefinition*(FName)>;

/** Одна стопка предметов, лежащая в ячейке. */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	int32 Count = 0;

	// Номер ячейки (0 — первая).
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	int32 Slot = 0;

	// Состояние штучных предметов (MaxStack = 1). У стопок не используется.

	// Патроны: в магазине или в оружии.
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	int32 Rounds = 0;

	// Прочность оружия, проценты.
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	float Durability = 100.0f;

	// Вставлен ли в оружие магазин.
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	bool bHasMagazine = false;

	// Будет ли следующий выстрел осечкой. Решает сервер заранее; хранится с оружием, чтобы решение нельзя было переиграть, сняв и надев его.
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	bool bNextShotMisfires = false;
};

/**
 * Ячейки рюкзака или ящика (docs/gdd/01_Системы_прототипа.md §2): любой предмет занимает ровно одну ячейку,
 * одинаковые предметы складываются в стопки до MaxStack. Только данные и правила, без сети и UI —
 * владелец (серверный компонент или ящик) вызывает эти функции и реплицирует результат.
 */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhInventoryGrid
{
	GENERATED_BODY()

	FRubezhInventoryGrid() = default;
	explicit FRubezhInventoryGrid(int32 InSlotCount) : SlotCount(FMath::Max(InSlotCount, 0)) {}

	const TArray<FRubezhInventoryEntry>& GetEntries() const { return Entries; }
	int32 GetSlotCount() const { return SlotCount; }
	bool IsEmpty() const { return Entries.IsEmpty(); }

	/** Какая стопка лежит в ячейке; INDEX_NONE — ячейка пуста. */
	int32 FindEntryAt(int32 Slot) const;

	bool IsSlotFree(int32 Slot) const { return Slot >= 0 && Slot < SlotCount && FindEntryAt(Slot) == INDEX_NONE; }

	/** Первая пустая ячейка; INDEX_NONE — всё занято. */
	int32 FindFreeSlot() const;

	/** Добавить: сначала доложить в неполные стопки, потом занять пустые ячейки. Возвращает, сколько не влезло. */
	int32 AddItem(FName ItemId, const FRubezhItemDefinition& Definition, int32 Count);

	/** Убрать штуки предмета, начиная с последней стопки. Возвращает, сколько убрано. */
	int32 RemoveItem(FName ItemId, int32 Count);

	/** Вынуть стопку целиком (например, чтобы выбросить). false — такой стопки нет. */
	bool RemoveEntry(int32 EntryIndex, FRubezhInventoryEntry& OutRemoved);

	/** Доложить в стопку до MaxStack. Возвращает, сколько доложено. */
	int32 AddToEntry(int32 EntryIndex, int32 Count, int32 MaxStack);

	/** Уменьшить стопку на Count штук (0 штук — стопка удаляется). false — такой стопки нет или в ней меньше. */
	bool ReduceEntry(int32 EntryIndex, int32 Count);

	/**
	 * Переложить стопку в ячейку, как в рюкзаке WoW: в пустую — переносится, на такой же предмет —
	 * докладывается (сколько влезет в стопку), на другой предмет — они меняются местами.
	 * false — ячейка вне рюкзака или такой стопки нет.
	 */
	bool MoveEntry(int32 EntryIndex, int32 NewSlot, FRubezhItemLookup Lookup);

	/** Положить стопку в указанную ячейку (например, снятую куртку). false — ячейка занята или вне рюкзака. */
	bool AddEntryAt(FName ItemId, int32 Count, int32 Slot, FRubezhItemLookup Lookup);

	/** Положить стопку со всем её состоянием в свободную ячейку (Slot). false — ячейка занята или вне рюкзака. */
	bool PlaceEntry(const FRubezhInventoryEntry& Entry, int32 Slot);

	/**
	 * Добавить стопку куда влезет, сохранив состояние штучного предмета (MaxStack = 1).
	 * Возвращает, сколько не влезло.
	 */
	int32 AddEntry(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Definition);

	/** Стопка для правки состояния (патроны, прочность). nullptr — такой стопки нет. */
	FRubezhInventoryEntry* GetEntryMutable(int32 EntryIndex) { return Entries.IsValidIndex(EntryIndex) ? &Entries[EntryIndex] : nullptr; }

	/**
	 * Поменять число ячеек (сменили рюкзак). Вещи, ячейки которых остались, лежат на месте,
	 * остальные переезжают в свободные. false — всё не влезет; тогда ничего не меняется.
	 */
	bool Resize(int32 NewSlotCount);

	/** Вынуть все стопки, для которых Predicate вернул true (порядок сохраняется). */
	TArray<FRubezhInventoryEntry> ExtractEntries(TFunctionRef<bool(const FRubezhInventoryEntry&)> Predicate);

	/** Набор ячеек ровно под эти стопки, разложенные по порядку с первой ячейки (например, вещи на теле). */
	static FRubezhInventoryGrid FromEntries(const TArray<FRubezhInventoryEntry>& InEntries);

	int32 CountItem(FName ItemId) const;

	/** Вес всего содержимого, кг: вместе с патронами в магазинах и оружии. Неизвестные предметы не учитываются. */
	float GetTotalWeight(FRubezhItemLookup Lookup) const;

	/** Вес патронов внутри магазина или оружия. Definition.AmmoItem — чем заряжено. */
	static float GetLoadedRoundsWeight(const FRubezhInventoryEntry& Entry, const FRubezhItemDefinition& Definition, FRubezhItemLookup Lookup);

private:
	UPROPERTY()
	int32 SlotCount = 0;

	UPROPERTY()
	TArray<FRubezhInventoryEntry> Entries;
};
