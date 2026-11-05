// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/ActorComponent.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhInventoryComponent.generated.h"

class APawn;
class APlayerState;
class ARubezhLootContainer;
class UAbilitySystemComponent;
class UDataTable;
class ULyraHealthComponent;
struct FHitResult;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRubezhInventoryChangedEvent);

/**
 * Рюкзак и надетое снаряжение игрока. Живёт на PlayerState, меняет только сервер и реплицирует владельцу.
 * Каждый предмет занимает одну ячейку (стопки — до MaxStack); число ячеек задаёт надетый рюкзак (без него — карманы).
 * После каждого изменения пересчитывает вес (CarriedWeight: рюкзак + надетое) и лимит (MaxCarryWeight: база + прибавка рюкзака).
 * Защиту надетого читает расчёт урона Lyra (через GetIncomingDamageMultiplier) и травмы.
 *
 * Обыск (ящики, тела, брошенные рюкзаки — ARubezhLootContainer): игрок просит начать обыск, сервер ждёт время обыска,
 * проверяет расстояние и присылает владельцу копию содержимого (LootGrid). Перенос вещей — запросы к серверу.
 * При смерти всё, что теряется (docs/gdd/00_Core_loop.md §4), ложится на землю «телом», которое может обыскать кто угодно.
 */
UCLASS(ClassGroup = Rubezh, Meta = (BlueprintSpawnableComponent))
class RUBEZHCORERUNTIME_API URubezhInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URubezhInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Найти рюкзак у PlayerState, контроллера или персонажа. */
	static URubezhInventoryComponent* FindForActor(const AActor* Actor);

	/** Только сервер. Возвращает, сколько не влезло. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Rubezh|Inventory")
	int32 AddItem(FName ItemId, int32 Count);

	/** Только сервер. Возвращает, сколько убрано. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Rubezh|Inventory")
	int32 RemoveItem(FName ItemId, int32 Count);

	/** Переложить стопку в другую ячейку рюкзака (на такой же предмет — доложить, на другой — поменять местами). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Rubezh|Inventory")
	void ServerMoveEntry(int32 EntryIndex, int32 NewSlot);

	/** Выбросить стопку на землю перед персонажем. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Rubezh|Inventory")
	void ServerDropEntry(int32 EntryIndex);

	/** Надеть предмет из рюкзака (что было в слоте — вернётся в рюкзак). */
	UFUNCTION(Server, Reliable)
	void ServerEquipEntry(int32 EntryIndex);

	/** Снять в рюкзак: в ячейку TargetSlot или (INDEX_NONE) куда влезет. */
	UFUNCTION(Server, Reliable)
	void ServerUnequip(ERubezhEquipSlot Slot, int32 TargetSlot);

	/** Снять и бросить на землю. Рюкзак с вещами падает вместе с ними — его можно обыскать или надеть обратно. */
	UFUNCTION(Server, Reliable)
	void ServerDropEquipped(ERubezhEquipSlot Slot);

	// --- Обыск ---

	/** Начать обыск (первый раз — с задержкой, повторный — сразу). Сервер сам проверяет расстояние. */
	UFUNCTION(Server, Reliable)
	void ServerStartSearch(ARubezhLootContainer* Container);

	/** Окно обыска закрыто. */
	UFUNCTION(Server, Reliable)
	void ServerCloseLoot();

	/** Забрать стопку из обыскиваемого в ячейку рюкзака TargetSlot (INDEX_NONE — куда влезет). ItemId — защита от устаревшего индекса. */
	UFUNCTION(Server, Reliable)
	void ServerTakeLoot(int32 LootEntryIndex, FName ItemId, int32 TargetSlot);

	/** Положить стопку из рюкзака в обыскиваемое (TargetSlot — ячейка или INDEX_NONE). */
	UFUNCTION(Server, Reliable)
	void ServerPutLoot(int32 EntryIndex, FName ItemId, int32 TargetSlot);

	/** Переложить внутри обыскиваемого. */
	UFUNCTION(Server, Reliable)
	void ServerMoveLootEntry(int32 LootEntryIndex, FName ItemId, int32 NewSlot);

	/** Забрать всё, что влезет. */
	UFUNCTION(Server, Reliable)
	void ServerTakeAllLoot();

	/** Надеть обыскиваемый брошенный рюкзак вместе с содержимым (свой рюкзак должен быть снят). */
	UFUNCTION(Server, Reliable)
	void ServerWearLootBag();

	/** Что сейчас открыто для обыска (nullptr — ничего) и его содержимое (копия от сервера). */
	ARubezhLootContainer* GetLootContainer() const { return LootContainer; }
	const FRubezhInventoryGrid& GetLootGrid() const { return LootGrid; }

	/** Идёт ли обыск этого контейнера и сколько секунд осталось. */
	bool IsSearching(const ARubezhLootContainer* Container) const;
	float GetSearchSecondsLeft() const;

	/** Только сервер (вызывает контейнер): прислать новую копию содержимого / закрыть обыск. */
	void RefreshLootView();
	void CloseLoot();

	// --- Чтение ---

	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	int32 CountItem(FName ItemId) const { return Grid.CountItem(ItemId); }

	/** Что надето в слоте (None — пусто). */
	FName GetEquipped(ERubezhEquipSlot Slot) const;
	const Rubezh::Equipment::FEquipped& GetEquippedItems() const { return Equipped; }

	/** Защита надетого для части тела. */
	Rubezh::Equipment::FProtection GetProtection(ERubezhBodyZone Zone) const;

	/** Ячеек без рюкзака (из настроек). */
	static int32 GetPocketSlots();

	/** Для расчёта урона Lyra: во сколько раз броня цели уменьшает урон от этого попадания. */
	static float GetIncomingDamageMultiplier(const UAbilitySystemComponent* Target, const FHitResult* Hit);

	/** Вес рюкзака и надетого, кг. */
	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	float GetTotalWeight() const;

	const FRubezhInventoryGrid& GetGrid() const { return Grid; }

	const FRubezhItemDefinition* FindDefinition(FName ItemId) const;

	/** Изменилось содержимое рюкзака, надетое или обыскиваемое. */
	UPROPERTY(BlueprintAssignable, Category = "Rubezh|Inventory")
	FRubezhInventoryChangedEvent OnInventoryChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_Grid();

	UFUNCTION()
	void OnRep_LootContainer(ARubezhLootContainer* PreviousContainer);

	UFUNCTION()
	void OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	UFUNCTION()
	void OnOwnerDeathStarted(AActor* OwningActor);

	void OnCarriedWeightChanged(const FOnAttributeChangeData& ChangeData);
	void RefreshOwnerWalkSpeed() const;

	void HandleGridChanged();
	void UpdateCarriedWeight() const;
	void ApplyStartingEquipment();
	void TrySyncWeightAttributes();
	void LogEquipFailure(Rubezh::Equipment::EResult Result) const;
	APawn* GetOwnerPawn() const;

	// Обыск (RubezhInventoryComponent_Loot.cpp).
	void FinishSearch();
	void CancelSearch();
	void OpenLoot(ARubezhLootContainer& Container);
	void CheckLootReach();
	bool CanReach(const ARubezhLootContainer& Container) const;
	/** Обыскиваемое, если оно есть и ещё в досягаемости. */
	ARubezhLootContainer* GetReachableLoot() const;
	void OpenWindowForLocalPlayer() const;
	void DropBodyOnDeath(const APawn& Pawn);
	double GetServerTime() const;

	FDelegateHandle CarriedWeightChangedHandle;
	FTimerHandle WeightSyncTimer;
	FTimerHandle SearchTimer;
	FTimerHandle LootReachTimer;
	TWeakObjectPtr<ULyraHealthComponent> BoundHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	FRubezhInventoryGrid Grid;

	// Надетое по слотам (порядок — Rubezh::Equipment::AllSlots).
	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	TArray<FName> Equipped;

	// Что открыто для обыска и копия его содержимого (видит только владелец).
	UPROPERTY(ReplicatedUsing = OnRep_LootContainer)
	TObjectPtr<ARubezhLootContainer> LootContainer;

	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	FRubezhInventoryGrid LootGrid;

	// Что сейчас обыскивается и когда обыск закончится (время сервера).
	UPROPERTY(Replicated)
	TObjectPtr<ARubezhLootContainer> SearchingContainer;

	UPROPERTY(Replicated)
	double SearchFinishTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<const UDataTable> ItemTable;
};
