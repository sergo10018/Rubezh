// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"

#include "RubezhWorldItem.generated.h"

class APawn;
class UStaticMeshComponent;
struct FRubezhInventoryEntry;
struct FRubezhItemDefinition;

/**
 * Стопка предметов, лежащая в мире: её можно подобрать (URubezhInteractionComponent),
 * в неё превращается выброшенное из рюкзака. Пока без моделей — коробка (чем тяжелее предмет, тем больше),
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

	/** Только сервер: положить на землю стопку из рюкзака со всем её состоянием (патроны, прочность). */
	static ARubezhWorldItem* SpawnEntryInFrontOf(APawn& Pawn, const FRubezhInventoryEntry& Entry);

	/** Только сервер: положить стопку в указанное место. */
	static ARubezhWorldItem* SpawnAt(UWorld& World, const FTransform& Transform, FName ItemId, int32 Count);

	/** Точка на земле под Point (или сам Point, если земли рядом нет). Ignore — кого не считать землёй. */
	static FVector FindGround(UWorld& World, const FVector& Point, const AActor* Ignore);

	/** Точка на земле перед персонажем, куда падает выброшенное (с небольшим разбросом). */
	static FVector FindDropLocation(const APawn& Pawn);

	/** Только сервер: чем стала бы эта стопка в рюкзаке. */
	FRubezhInventoryEntry MakeEntry() const;

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

	// Состояние штучного предмета: знает только сервер.
	int32 Rounds = 0;
	float Durability = 100.0f;
	bool bHasMagazine = false;
	bool bNextShotMisfires = false;
};
