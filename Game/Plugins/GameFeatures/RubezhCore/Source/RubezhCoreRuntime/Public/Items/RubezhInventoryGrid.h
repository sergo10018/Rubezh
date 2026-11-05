// РУБЕЖ: Земля после разлома

#pragma once

#include "Items/RubezhItemDefinition.h"
#include "Templates/Function.h"

#include "RubezhInventoryGrid.generated.h"

/** Поиск описания предмета по идентификатору (обычно — строка DataTable). nullptr — предмет неизвестен. */
using FRubezhItemLookup = TFunctionRef<const FRubezhItemDefinition*(FName)>;

/** Одна стопка предметов, лежащая в сетке. */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	int32 Count = 0;

	// Левая верхняя клетка.
	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	FIntPoint Position = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Rubezh|Inventory")
	bool bRotated = false;
};

/**
 * Сетка рюкзака (docs/gdd/01_Системы_прототипа.md §2): предметы занимают прямоугольники клеток,
 * одинаковые предметы складываются в стопки до MaxStack. Только данные и правила, без сети и UI —
 * владелец (серверный компонент) вызывает эти функции и реплицирует результат.
 */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhInventoryGrid
{
	GENERATED_BODY()

	FRubezhInventoryGrid() = default;
	explicit FRubezhInventoryGrid(FIntPoint InSize) : Size(InSize) {}

	const TArray<FRubezhInventoryEntry>& GetEntries() const { return Entries; }
	FIntPoint GetSize() const { return Size; }

	/** Свободна ли область (и целиком ли внутри сетки). IgnoreIndex — стопка, которую сейчас переносим. */
	bool IsAreaFree(FIntPoint Position, FIntPoint Footprint, FRubezhItemLookup Lookup, int32 IgnoreIndex = INDEX_NONE) const;

	/** Первая свободная позиция: сначала без поворота, потом с поворотом. */
	bool FindFreeSpot(const FRubezhItemDefinition& Definition, FRubezhItemLookup Lookup, FIntPoint& OutPosition, bool& bOutRotated) const;

	/** Добавить: сначала доложить в неполные стопки, потом занять свободные места. Возвращает, сколько не влезло. */
	int32 AddItem(FName ItemId, const FRubezhItemDefinition& Definition, int32 Count, FRubezhItemLookup Lookup);

	/** Убрать штуки предмета, начиная с последней стопки. Возвращает, сколько убрано. */
	int32 RemoveItem(FName ItemId, int32 Count);

	/** Вынуть стопку целиком (например, чтобы выбросить). false — такой стопки нет. */
	bool RemoveEntry(int32 EntryIndex, FRubezhInventoryEntry& OutRemoved);

	/** Переложить стопку на новое место (с поворотом или без). false — место занято или вне сетки. */
	bool MoveEntry(int32 EntryIndex, FIntPoint NewPosition, bool bRotated, FRubezhItemLookup Lookup);

	/** Положить стопку в указанное место (например, снятую куртку). false — место занято или вне сетки. */
	bool AddEntryAt(FName ItemId, int32 Count, FIntPoint Position, bool bRotated, FRubezhItemLookup Lookup);

	/**
	 * Поменять размер сетки (сменили рюкзак). Вещи, которые помещаются на своих местах, остаются там,
	 * остальные раскладываются заново. false — всё не влезет; тогда сетка не меняется.
	 */
	bool Resize(FIntPoint NewSize, FRubezhItemLookup Lookup);

	int32 CountItem(FName ItemId) const;

	/** Вес всего содержимого, кг. Неизвестные предметы не учитываются. */
	float GetTotalWeight(FRubezhItemLookup Lookup) const;

private:
	FIntPoint GetEntryFootprint(const FRubezhInventoryEntry& Entry, FRubezhItemLookup Lookup) const;

	UPROPERTY()
	FIntPoint Size = FIntPoint(6, 8);

	UPROPERTY()
	TArray<FRubezhInventoryEntry> Entries;
};
