// РУБЕЖ: Земля после разлома

#pragma once

#include "GameplayEffect.h"

#include "RubezhHealthEffects.generated.h"

/** Мгновенный урон здоровью Lyra (атрибут Damage). Величина — SetByCaller.Rubezh.Damage. Применяет кровотечение. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhGE_InjuryDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URubezhGE_InjuryDamage();
};

/** Мгновенное лечение здоровья Lyra (атрибут Healing). Величина — SetByCaller.Rubezh.Healing. Применяют аптечки. */
UCLASS()
class RUBEZHCORERUNTIME_API URubezhGE_Heal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	URubezhGE_Heal();
};
