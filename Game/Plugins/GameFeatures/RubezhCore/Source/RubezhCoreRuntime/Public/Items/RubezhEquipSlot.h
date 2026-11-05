// РУБЕЖ: Земля после разлома

#pragma once

#include "CoreMinimal.h"

#include "RubezhEquipSlot.generated.h"

/**
 * Куда надевается предмет (docs/gdd/01_Системы_прототипа.md §2, «Слоты экипировки»).
 * Слоты оружия и разгрузки — позже. None — предмет не надевается.
 */
UENUM(BlueprintType)
enum class ERubezhEquipSlot : uint8
{
	None,
	Head,		// шапка, каска, шлем
	Armor,		// бронежилет
	Top,		// одежда: верх
	Bottom,		// одежда: низ
	Backpack	// рюкзак: размер сетки и прибавка к лимиту веса
};

/** Часть тела, в которую пришло попадание. */
enum class ERubezhBodyZone : uint8
{
	Head,
	Torso,
	Arms,
	Legs
};
