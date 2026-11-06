// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/PawnComponent.h"

#include "RubezhSafeZoneComponent.generated.h"

class UAbilitySystemComponent;

/**
 * Следит, не вошёл ли персонаж в безопасную зону (ARubezhSafeZone), и пока он там, запрещает стрелять, бросать гранату и бить.
 * Запрет ставится и на сервере, и у своего клиента — блокировка способностей не реплицируется, а клиент иначе «предскажет»
 * выстрел, который сервер отвергнет. Добавляет URubezhComponentInjector.
 */
UCLASS(ClassGroup = Rubezh)
class RUBEZHCORERUNTIME_API URubezhSafeZoneComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	URubezhSafeZoneComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool IsLocalPlayerPawn() const;
	void SetInside(bool bNowInside);

	// ASC живёт на PlayerState и переживает персонажа: запрет обязательно снимается в EndPlay.
	TWeakObjectPtr<UAbilitySystemComponent> BlockedAbilitySystem;
	bool bInside = false;
};
