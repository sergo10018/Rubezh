// РУБЕЖ: Земля после разлома

#pragma once

#include "GameplayEffect.h"

#include "RubezhStaminaEffects.generated.h"

/** Постоянное восстановление выносливости. Выдаётся набором способностей при появлении персонажа. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhGE_StaminaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URubezhGE_StaminaRegen();
};

/** Расход выносливости в спринте. Величина за период задаётся через SetByCaller.Rubezh.StaminaDelta. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhGE_SprintStaminaDrain : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URubezhGE_SprintStaminaDrain();
};
