// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhLootContainer.generated.h"

class UStaticMeshComponent;
class URubezhInventoryComponent;

/**
 * То, что можно обыскать: ящик на карте, тело погибшего (вещи, выпавшие при смерти) или брошенный рюкзак.
 * Содержимое знает только сервер; тем, кто сейчас обыскивает, он присылает копию через их URubezhInventoryComponent.
 * Первый обыск занимает SearchSeconds (docs/gdd/01_Системы_прототипа.md §2: обыск — уязвимость), повторный — сразу.
 * Ящик на карте: поставить актор, задать Loot Table (military, medical, civil — data/loot_tables.csv) и Slot Count.
 * Пока без моделей — цветная коробка.
 */
UCLASS(Blueprintable)
class RUBEZHCORERUNTIME_API ARubezhLootContainer : public AActor
{
	GENERATED_BODY()

public:
	ARubezhLootContainer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Только сервер: тело или брошенный рюкзак на земле в точке Location. BagItemId — какой рюкзак (None — тело). */
	static ARubezhLootContainer* SpawnDropped(UWorld& World, const FVector& Location, float Yaw, const FText& Name,
		const FRubezhInventoryGrid& Contents, FName BagItemId, float LifetimeSeconds);

	/** Только сервер: ящик с лутом из таблицы LootTableName (для проверок). */
	static ARubezhLootContainer* SpawnCrate(UWorld& World, const FTransform& Transform, FName LootTableName, int32 Slots);

	const FText& GetDisplayName() const { return DisplayName; }
	float GetSearchSeconds() const { return SearchSeconds; }

	/** Не None — это брошенный рюкзак: его можно надеть вместе с содержимым. */
	FName GetBagItemId() const { return BagItemId; }

	/** Середина коробки — куда «смотреть» и откуда мерить расстояние. */
	FVector GetFocusLocation() const;

	/** Только сервер. */
	const FRubezhInventoryGrid& GetContents() const { return Contents; }

	/**
	 * Только сервер: изменить содержимое. Change возвращает, изменилось ли что-то; тогда все, кто обыскивает,
	 * получают новую копию, а пустое тело или рюкзак исчезает (рюкзак остаётся на земле обычным предметом).
	 */
	bool ModifyContents(TFunctionRef<bool(FRubezhInventoryGrid&)> Change);

	/** Только сервер: рюкзак надели целиком — убрать его с земли. */
	void RemoveAsWornBag();

	void AddViewer(URubezhInventoryComponent& Viewer);
	void RemoveViewer(const URubezhInventoryComponent& Viewer);

	bool WasSearchedBy(const URubezhInventoryComponent& Searcher) const;
	void MarkSearchedBy(URubezhInventoryComponent& Searcher);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Название в подсказке и в окне обыска.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Rubezh|Loot")
	FText DisplayName;

	// Тип места из data/loot_tables.csv (military, medical, civil). Пусто — ящик остаётся пустым.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot")
	FName LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot", Meta = (ClampMin = 1, ClampMax = 48))
	int32 SlotCount = 12;

	// Сколько длится первый обыск, секунд.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Rubezh|Loot", Meta = (ClampMin = 0))
	float SearchSeconds = 2.0f;

	// Исчезнуть, когда всё забрали (тела и брошенные рюкзаки).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Loot")
	bool bDestroyWhenEmpty = false;

	// Размер коробки, см, и её цвет (временный вид без модели).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Visual, Category = "Rubezh|Loot")
	FVector BoxSizeCm = FVector(60.0f, 40.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Visual, Category = "Rubezh|Loot")
	FLinearColor BoxColor = FLinearColor(0.08f, 0.1f, 0.04f);

	// Персонажи упираются в коробку (ящик) или проходят сквозь (тело, рюкзак).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Visual, Category = "Rubezh|Loot")
	bool bBlocksCharacters = true;

private:
	UFUNCTION()
	void OnRep_Visual();

	void UpdateVisual();
	void FillFromLootTable();
	void NotifyViewers();

	UPROPERTY(Replicated)
	FName BagItemId;

	// Наполняется при старте игры (из таблицы лута) — в файл карты не сохраняется.
	UPROPERTY(Transient)
	FRubezhInventoryGrid Contents;

	TArray<TWeakObjectPtr<URubezhInventoryComponent>> Viewers;
	TArray<TWeakObjectPtr<URubezhInventoryComponent>> SearchedBy;

	UPROPERTY(VisibleAnywhere, Category = "Rubezh|Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
