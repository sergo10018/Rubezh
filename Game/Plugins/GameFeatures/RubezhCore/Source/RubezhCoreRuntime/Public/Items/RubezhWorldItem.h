// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"

#include "RubezhWorldItem.generated.h"

class APawn;
class UStaticMeshComponent;
struct FRubezhItemDefinition;

/**
 * Стопка предметов, лежащая в мире: её можно подобрать (URubezhInteractionComponent),
 * в неё превращается выброшенное из рюкзака. Пока без моделей — коробка размером с предмет в сетке,
 * окрашенная по категории. Количество меняет только сервер.
 * Можно ставить на карту вручную: в свойствах задать Item Id (строка из DT_Items) и Count.
 */
UCLASS(Blueprintable)
class RUBEZHCORERUNTIME_API ARubezhWorldItem : public AActor
{
	GENERATED_BODY()

public:
	ARubezhWorldItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Только сервер: положить стопку на землю перед персонажем. */
	static ARubezhWorldItem* SpawnInFrontOf(APawn& Pawn, FName ItemId, int32 Count);

	FName GetItemId() const { return ItemId; }
	int32 GetCount() const { return Count; }

	/** Только сервер. 0 — стопка подобрана целиком, актор удаляется. */
	void SetCount(int32 NewCount);

	/** Середина коробки — куда «смотреть» и где показывать подсказку. */
	FVector GetFocusLocation() const;

	/** Описание предмета из таблицы; nullptr — предмет неизвестен. */
	const FRubezhItemDefinition* FindDefinition() const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Item, Category = "Rubezh|Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Item, Category = "Rubezh|Item", Meta = (ClampMin = 1))
	int32 Count = 1;

private:
	UFUNCTION()
	void OnRep_Item();

	void UpdateVisual();

	UPROPERTY(VisibleAnywhere, Category = "Rubezh|Item")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
