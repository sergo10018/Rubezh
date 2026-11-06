// РУБЕЖ: Земля после разлома

#pragma once

#include "Engine/DataTable.h"
#include "Health/RubezhInjury.h"
#include "Items/RubezhEquipSlot.h"
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

	// Сколько штук помещается в одну ячейку-стопку (любой предмет занимает одну ячейку).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item", Meta = (ClampMin = 1))
	int32 MaxStack = 1;

	// --- Магазин (у остальных None и 0) ---

	// Какие патроны в него набиваются.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Magazine")
	FName AmmoItem;

	// Сколько патронов вмещает. Больше нуля — предмет считается магазином.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Magazine", Meta = (ClampMin = 0))
	int32 MagazineCapacity = 0;

	// --- Лечение (только для медицины; у остальных нули) ---

	// Сколько секунд длится применение. 0 — предмет нельзя «использовать».
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float UseSeconds = 0.0f;

	// Сколько процентов максимального здоровья восстанавливает сразу по окончании применения.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0, ClampMax = 100))
	float HealPercent = 0.0f;

	// Сколько процентов добавляется ещё, когда пройдёт DelayedHealSeconds после применения (действие аптечки).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0, ClampMax = 100))
	float DelayedHealPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float DelayedHealSeconds = 0.0f;

	/** Всё здоровье, которое даст предмет (сразу и потом), в процентах от максимума. */
	float GetTotalHealPercent() const { return HealPercent + DelayedHealPercent; }

	// Какую травму лечит.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical")
	ERubezhInjury Cures = ERubezhInjury::None;

	// На сколько секунд снимает боль.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Medical", Meta = (ClampMin = 0))
	float PainReliefSeconds = 0.0f;

	// --- Снаряжение (броня, одежда, рюкзаки; у остальных None и нули) ---

	// Куда надевается.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment")
	ERubezhEquipSlot EquipSlot = ERubezhEquipSlot::None;

	// На сколько процентов снижает урон от попаданий в закрытые части тела.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment", Meta = (ClampMin = 0, ClampMax = 95))
	float Protection = 0.0f;

	// На сколько процентов снижает шанс кровотечения от попадания в закрытые части тела.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment", Meta = (ClampMin = 0, ClampMax = 100))
	float BleedProtection = 0.0f;

	// Утепление (пока ни на что не влияет — пригодится, когда появится климат).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment", Meta = (ClampMin = 0))
	float Warmth = 0.0f;

	// Рюкзак: сколько в нём ячеек.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment", Meta = (ClampMin = 0))
	int32 BackpackSlots = 0;

	// Рюкзак: прибавка к лимиту переносимого веса, кг (хорошая подвесная система — меньше устаёшь).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Item|Equipment", Meta = (ClampMin = 0))
	float CarryBonusKg = 0.0f;
};
