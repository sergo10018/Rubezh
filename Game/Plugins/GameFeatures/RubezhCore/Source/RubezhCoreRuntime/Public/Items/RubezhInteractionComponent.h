// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/PawnComponent.h"

#include "RubezhInteractionComponent.generated.h"

class ARubezhLootContainer;
class ARubezhWorldItem;
class URubezhPickupPrompt;

/**
 * Подбор предметов с земли и обыск (ящики, тела, брошенные рюкзаки). Живёт на персонаже игрока
 * (добавляет URubezhComponentInjector). У своего игрока каждый кадр ищет, на что он смотрит, и показывает подсказку;
 * по клавише взаимодействия просит сервер подобрать или начать обыск. Сервер сам проверяет расстояние и место в рюкзаке.
 */
UCLASS(ClassGroup = Rubezh, Meta = (BlueprintSpawnableComponent))
class RUBEZHCORERUNTIME_API URubezhInteractionComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	URubezhInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Клавиша взаимодействия нажата: подобрать предмет или обыскать то, на что смотрим. */
	void Interact();

	/** Предмет на земле или контейнер, на который смотрит игрок (nullptr — ничего). */
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Коротко показать сообщение под прицелом (например, «Осечка»). Только у своего игрока. */
	void ShowNotice(const FString& Text);

private:
	FString Notice;
	double NoticeEndTime = 0.0;

	UFUNCTION(Server, Reliable)
	void ServerPickUp(ARubezhWorldItem* Item);

	bool IsLocalPlayerPawn() const;
	AActor* FindFocusedActor() const;
	void UpdatePrompt();
	void ShowPrompt(const FText& Name, const FString& Action, bool bWarning);
	void HidePrompt();

	/** Подсказка для контейнера: «E — обыскать» или сколько осталось обыскивать. */
	FString DescribeContainerAction(const ARubezhLootContainer& Container) const;

	/** Сколько штук предмета сейчас влезет в рюкзак (по копии содержимого, которое знает клиент). */
	int32 CountFitting(const ARubezhWorldItem& Item) const;

	TWeakObjectPtr<AActor> FocusedActor;

	UPROPERTY(Transient)
	TObjectPtr<URubezhPickupPrompt> Prompt;
};
