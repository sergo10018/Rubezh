// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/ActorComponent.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryGrid.h"
#include "Items/RubezhTradeRules.h"
#include "Items/RubezhWeaponRules.h"

#include "RubezhInventoryComponent.generated.h"

class APawn;
class APlayerState;
class ARubezhLootContainer;
class ARubezhTrader;
class UAbilitySystemComponent;
class UDataTable;
class ULyraHealthComponent;
class ULyraInventoryItemInstance;
class ULyraQuickBarComponent;
struct FGameplayEventData;
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

	/** Только сервер: добавить стопку с её состоянием (патроны, прочность). Возвращает, сколько не влезло. */
	int32 AddEntry(const FRubezhInventoryEntry& Entry);

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

	/** Починить надетое у бронника. */
	UFUNCTION(Server, Reliable)
	void ServerRepairEquipped(ERubezhEquipSlot Slot);

	// --- Оружие (RubezhInventoryComponent_Weapons.cpp) ---

	/** Взять оружие из рюкзака в слот (INDEX_NONE — подходящий сам). */
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(int32 EntryIndex, int32 WeaponSlot);

	/** Убрать оружие из слота в ячейку Cell (INDEX_NONE — куда влезет). */
	UFUNCTION(Server, Reliable)
	void ServerUnequipWeapon(int32 WeaponSlot, int32 Cell);

	/** Бросить оружие из слота на землю. */
	UFUNCTION(Server, Reliable)
	void ServerDropWeapon(int32 WeaponSlot);

	/** Начать набивать магазин патронами из рюкзака (по одному, пока открыто окно). */
	UFUNCTION(Server, Reliable)
	void ServerStartLoadingMagazine(int32 EntryIndex);

	/** Перестать набивать магазин. */
	UFUNCTION(Server, Reliable)
	void ServerStopLoadingMagazine();

	/** Разрядить магазин в рюкзак. */
	UFUNCTION(Server, Reliable)
	void ServerUnloadMagazine(int32 EntryIndex);

	/** Оружие в слотах (длина Rubezh::Weapons::SlotCount, пустой ItemId — слот свободен). */
	const Rubezh::Weapons::FWeaponSlots& GetWeaponSlots() const { return WeaponSlots; }

	/** Только сервер (для проверки): задать прочность оружия в слоте. */
	void SetWeaponDurability(int32 WeaponSlot, float Durability);

	/** Только сервер (для проверки): дать заряженное оружие в слот. */
	bool GiveLoadedWeapon(FName ItemId);

	/** Только сервер (для проверки): какой слот оружия сейчас в руках, INDEX_NONE — никакой. */
	int32 GetActiveWeaponSlot() const;

	/** Для расчёта урона Lyra: урон пули с учётом дистанции; меньше нуля — оружие не наше. */
	static float GetWeaponDamage(const UObject* LyraItem, double DistanceCm);

	/** Выстрел принят (сервер и клиент стрелявшего). false — осечка. */
	static bool HandleShotCommitted(UObject* LyraItem, APawn* Pawn);

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

	// --- Торговля (RubezhInventoryComponent_Trade.cpp) ---

	/** Начать сделку с торговцем: сервер проверит расстояние и откроет окно. */
	UFUNCTION(Server, Reliable)
	void ServerOpenTrader(ARubezhTrader* Trader);

	UFUNCTION(Server, Reliable)
	void ServerCloseTrader();

	/** Купить Count штук у открытого торговца. */
	UFUNCTION(Server, Reliable)
	void ServerBuyItem(FName ItemId, int32 Count);

	/** Продать открытому торговцу стопку из ячейки рюкзака. ItemId — защита от устаревшего запроса. */
	UFUNCTION(Server, Reliable)
	void ServerSellEntry(int32 Cell, FName ItemId);

	/** Починить у оружейника оружие из ячейки рюкзака / из слота. */
	UFUNCTION(Server, Reliable)
	void ServerRepairEntry(int32 Cell, FName ItemId);

	UFUNCTION(Server, Reliable)
	void ServerRepairWeapon(int32 WeaponSlot);

	/** Торговец, с которым идёт сделка (nullptr — никакого). */
	ARubezhTrader* GetTraderInUse() const { return TraderInUse; }

	/** Билеты на счету. При смерти не теряются. */
	int32 GetMoney() const { return Money; }

	/** Только сервер (для проверки): задать счёт. */
	void SetMoney(int32 NewMoney);

	/** Какое оружие лежит в слоте (None — пусто). */
	FName GetWeaponInSlot(int32 WeaponSlot) const;

	/** Можно ли почистить оружие в слоте: оно есть и прочность ниже потолка чистки. */
	bool CanCleanWeapon(int32 WeaponSlot) const;

	/** Только сервер: чистка закончена. false — чистить нечего, набор не тратится. */
	bool CleanWeapon(int32 WeaponSlot, float CleanPercent);

	// --- Чтение ---

	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	int32 CountItem(FName ItemId) const { return Grid.CountItem(ItemId); }

	/** Что надето в слоте (None — пусто). */
	FName GetEquipped(ERubezhEquipSlot Slot) const;
	const Rubezh::Equipment::FEquipped& GetEquippedItems() const { return Equipped; }

	/** Защита надетого для части тела с учётом износа. */
	Rubezh::Equipment::FProtection GetProtection(ERubezhBodyZone Zone) const;

	/** Прочность надетого в слоте, проценты. */
	float GetEquippedDurability(ERubezhEquipSlot Slot) const;

	/** Только сервер: попадание изнашивает всё, что закрывает часть тела. DamageTaken — урон после брони. */
	void ApplyHitWear(ERubezhBodyZone Zone, float DamageTaken);

	/** Только сервер (для проверки): задать прочность надетого. */
	void SetEquippedDurability(ERubezhEquipSlot Slot, float Durability);

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

	// Торговля (RubezhInventoryComponent_Trade.cpp).
	UFUNCTION()
	void OnRep_TraderInUse(ARubezhTrader* PreviousTrader);

	UFUNCTION()
	void OnRep_Money();

	void CloseTrader();
	void CheckTraderReach();
	bool CanReachTrader(const ARubezhTrader& Trader) const;
	/** Открытый торговец, если он есть и ещё в досягаемости. */
	const ARubezhTrader* GetReachableTrader() const;
	bool CanRepairHere(FName ItemId) const;
	void RepairEntry(FRubezhInventoryEntry& Entry);
	void LogTrade(const TCHAR* Action, FName ItemId, Rubezh::Trade::EResult Result) const;

	// Оружие (RubezhInventoryComponent_Weapons.cpp).
	void ApplyStartingWeapons();
	void BindReloadEvent();
	void UnbindReloadEvent();
	void HandleReloadDone(const FGameplayEventData* EventData);
	void FinishReload();
	void HandleWeaponsChanged();
	void RebuildWeaponInstances();
	void TryCreateWeaponInstances();
	void ClearWeaponInstances();
	void CreateWeaponInstance(int32 WeaponSlot);
	void SyncWeaponTags() const;
	void RollNextShot(FRubezhInventoryEntry& Weapon) const;
	bool IsOwnerDead() const;
	void HandleServerShot(ULyraInventoryItemInstance& Item);
	void ApplyRecoil(const FRubezhWeaponDefinition& Weapon, APawn& Pawn);
	void LoadNextRound();
	ULyraQuickBarComponent* FindQuickBar() const;
	const FRubezhWeaponDefinition* FindWeaponByInstance(const ULyraInventoryItemInstance& Item) const;
	int32 FindWeaponSlot(const ULyraInventoryItemInstance& Item) const;
	double GetServerTime() const;

	FDelegateHandle CarriedWeightChangedHandle;
	FTimerHandle WeightSyncTimer;
	FTimerHandle SearchTimer;
	FTimerHandle LootReachTimer;
	FTimerHandle WeaponInstanceTimer;
	FTimerHandle MagazineLoadTimer;
	FDelegateHandle ReloadEventHandle;
	int32 ActiveWeaponSlotBeforeRebuild = INDEX_NONE;
	int32 LoadingMagazineCell = INDEX_NONE;
	int32 ReloadingWeaponSlot = INDEX_NONE;
	int32 WeaponInstanceRetries = 0;
	int32 ShotsInBurst = 0;
	double LastShotTime = 0.0;
	TWeakObjectPtr<ULyraHealthComponent> BoundHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	FRubezhInventoryGrid Grid;

	// Надетое по слотам (порядок — Rubezh::Equipment::AllSlots).
	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	TArray<FName> Equipped;

	// Прочность надетого, проценты; индексы как у Equipped.
	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	TArray<float> EquippedDurability;

	// Оружие в слотах. Числа на экземплярах Lyra (патроны для стрельбы и счётчика) — лишь их копия.
	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	TArray<FRubezhInventoryEntry> WeaponSlots;

	// Предметы Lyra, созданные для оружия в слотах (только сервер); индексы как у WeaponSlots.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULyraInventoryItemInstance>> WeaponInstances;

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

	// Билеты на счету: не вещь, не весят и при смерти не теряются. Постоянными станут с бэкендом (Этап 3).
	UPROPERTY(ReplicatedUsing = OnRep_Money)
	int32 Money = 0;

	// С кем идёт сделка.
	UPROPERTY(ReplicatedUsing = OnRep_TraderInUse)
	TObjectPtr<ARubezhTrader> TraderInUse;

	FTimerHandle TraderReachTimer;

	UPROPERTY(Transient)
	TObjectPtr<const UDataTable> ItemTable;
};
