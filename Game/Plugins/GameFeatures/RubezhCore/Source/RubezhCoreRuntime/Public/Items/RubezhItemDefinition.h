// РУБЕЖ: Земля после разлома

#pragma once

#include "Engine/DataTable.h"
#include "Health/RubezhInjury.h"
#include "Items/RubezhItemCategory.h"

#include "RubezhItemDefinition.generated.h"

/**
 * Описание предмета — строка DataTable, импортируется из data/items.csv.
 * Имя строки — идентификатор предмета (ammo_9x18, medkit_small, ...).
 */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhItemDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item")
	ERubezhItemCategory Category = ERubezhItemCategory::Junk;

	// Вес одной штуки, кг.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item", Meta = (ClampMin = 0))
	float WeightKg = 0.0f;

	// Размер в клетках сетки (в неповёрнутом виде).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item", Meta = (ClampMin = 1))
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item", Meta = (ClampMin = 1))
	int32 Height = 1;

	// Сколько штук помещается в одну клетку-стопку.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item", Meta = (ClampMin = 1))
	int32 MaxStack = 1;

	// --- Лечение (только для медицины; у остальных нули) ---

	// Сколько секунд длится применение. 0 — предмет нельзя «использовать».
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float UseSeconds = 0.0f;

	// Сколько здоровья восстанавливает.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float HealAmount = 0.0f;

	// Какую травму лечит.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical")
	ERubezhInjury Cures = ERubezhInjury::None;

	// На сколько секунд снимает боль.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float PainReliefSeconds = 0.0f;

	FIntPoint GetFootprint(bool bRotated) const
	{
		return bRotated ? FIntPoint(Height, Width) : FIntPoint(Width, Height);
	}
};
