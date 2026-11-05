// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/PawnComponent.h"

#include "RubezhInteractionComponent.generated.h"

class ARubezhWorldItem;
class URubezhPickupPrompt;

/**
 * Подбор предметов с земли. Живёт на персонаже игрока (добавляет URubezhComponentInjector).
 * У своего игрока каждый кадр ищет предмет, на который он смотрит, и показывает подсказку;
 * по клавише взаимодействия просит сервер подобрать. Сервер сам проверяет расстояние и место в рюкзаке.
 */
UCLASS(ClassGroup = Rubezh, Meta = (BlueprintSpawnableComponent))
class RUBEZHCORERUNTIME_API URubezhInteractionComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	URubezhInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Клавиша взаимодействия нажата: подобрать предмет, на который смотрим. */
	void Interact();

	ARubezhWorldItem* GetFocusedItem() const { return FocusedItem.Get(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerPickUp(ARubezhWorldItem* Item);

	bool IsLocalPlayerPawn() const;
	ARubezhWorldItem* FindFocusedItem() const;
	void UpdatePrompt();

	/** Сколько штук предмета сейчас влезет в рюкзак (по копии содержимого, которое знает клиент). */
	int32 CountFitting(const ARubezhWorldItem& Item) const;

	TWeakObjectPtr<ARubezhWorldItem> FocusedItem;

	UPROPERTY(Transient)
	TObjectPtr<URubezhPickupPrompt> Prompt;
};
