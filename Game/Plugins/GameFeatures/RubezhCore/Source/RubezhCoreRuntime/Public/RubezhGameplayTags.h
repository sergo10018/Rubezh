// РУБЕЖ: Земля после разлома

#pragma once

#include "NativeGameplayTags.h"

namespace RubezhGameplayTags
{
	/** Персонаж бежит спринтом (выдаётся способностью спринта, пока она активна). */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Sprinting);

	/** SetByCaller: изменение выносливости за один период эффекта. */
	RUBEZHCORERUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaDelta);
}
