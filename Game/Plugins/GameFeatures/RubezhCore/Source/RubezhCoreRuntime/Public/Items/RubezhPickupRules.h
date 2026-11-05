// РУБЕЖ: Земля после разлома
// Правила подбора предметов с земли: дотягивается ли игрок и на какой предмет он смотрит. Только математика — проверяется тестами.

#pragma once

#include "CoreMinimal.h"

namespace Rubezh::Pickup
{
	// С какого расстояния (от центра персонажа до предмета, см) можно подобрать предмет.
	inline constexpr float ReachCm = 200.0f;

	// Запас для сервера: позиции у клиента и сервера чуть расходятся из-за задержки.
	inline constexpr float ServerReachToleranceCm = 75.0f;

	// Насколько точно надо смотреть на предмет: угол от направления взгляда, градусы.
	inline constexpr float MaxViewAngleDeg = 30.0f;

	// Предметы, выброшенные из рюкзака: на каком расстоянии перед персонажем они падают (см).
	inline constexpr float DropForwardCm = 80.0f;

	/** Дотягивается ли персонаж до предмета. bServerCheck добавляет запас на задержку сети. */
	RUBEZHCORERUNTIME_API bool IsWithinReach(const FVector& PawnLocation, const FVector& ItemLocation, bool bServerCheck);

	/**
	 * Оценка предмета как цели подбора: угол (градусы) между взглядом и направлением на предмет.
	 * Меньше — лучше. Пусто, если предмет вне досягаемости или игрок смотрит не туда.
	 */
	RUBEZHCORERUNTIME_API TOptional<float> ScoreCandidate(const FVector& ViewLocation, const FVector& ViewDirection,
		const FVector& PawnLocation, const FVector& ItemLocation);
}
