// РУБЕЖ: Земля после разлома

#pragma once

#include "Survival/RubezhEncumbrance.h"

class ACharacter;

/**
 * Единственное место, где считается скорость ходьбы: база × перегруз × спринт.
 * Вызывается при начале и конце спринта и при изменении переносимого веса — и на сервере, и у владельца,
 * чтобы обе стороны получили одно и то же значение.
 */
namespace Rubezh::Movement
{
	RUBEZHCORERUNTIME_API float ComputeWalkSpeed(float BaseSpeed, ERubezhEncumbranceLevel Level, bool bSprinting);

	/** Пересчитать MaxWalkSpeed персонажа по его атрибутам выживания и тегу спринта. */
	RUBEZHCORERUNTIME_API void RefreshWalkSpeed(ACharacter* Character);
}
