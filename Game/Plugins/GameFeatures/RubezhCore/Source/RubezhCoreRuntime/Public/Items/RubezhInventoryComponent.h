// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/ActorComponent.h"
#include "Items/RubezhEquipment.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhInventoryComponent.generated.h"

class APawn;
class APlayerState;
class UAbilitySystemComponent;
class UDataTable;
struct FHitResult;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRubezhInventoryChangedEvent);

/**
 * Рюкзак и надетое снаряжение игрока. Живёт на PlayerState, меняет только сервер и реплицирует владельцу.
 * Размер сетки задаёт надетый рюкзак (без него — карманы). После каждого изменения пересчитывает вес
 * (CarriedWeight: сетка + надетое) и лимит (MaxCarryWeight: база + прибавка рюкзака) в URubezhSurvivalSet.
 * Защиту надетого читает расчёт урона Lyra (через GetIncomingDamageMultiplier) и травмы.
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

	/** Клиент просит переложить стопку; сервер проверяет и решает. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Rubezh|Inventory")
	void ServerMoveEntry(int32 EntryIndex, FIntPoint NewPosition, bool bRotated);

	/** Клиент просит выбросить стопку на землю перед персонажем. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Rubezh|Inventory")
	void ServerDropEntry(int32 EntryIndex);

	/** Надеть предмет из сетки (что было в слоте — вернётся в сетку). */
	UFUNCTION(Server, Reliable)
	void ServerEquipEntry(int32 EntryIndex);

	/** Снять в сетку: на место Position (bAutoPlace = false) или куда влезет. */
	UFUNCTION(Server, Reliable)
	void ServerUnequip(ERubezhEquipSlot Slot, FIntPoint Position, bool bRotated, bool bAutoPlace);

	/** Снять и бросить на землю. */
	UFUNCTION(Server, Reliable)
	void ServerDropEquipped(ERubezhEquipSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	int32 CountItem(FName ItemId) const { return Grid.CountItem(ItemId); }

	/** Что надето в слоте (None — пусто). */
	FName GetEquipped(ERubezhEquipSlot Slot) const;
	const Rubezh::Equipment::FEquipped& GetEquippedItems() const { return Equipped; }

	/** Защита надетого для части тела. */
	Rubezh::Equipment::FProtection GetProtection(ERubezhBodyZone Zone) const;

	/** Сетка без рюкзака (из настроек). */
	static FIntPoint GetPocketsSize();

	/** Для расчёта урона Lyra: во сколько раз броня цели уменьшает урон от этого попадания. */
	static float GetIncomingDamageMultiplier(const UAbilitySystemComponent* Target, const FHitResult* Hit);

	/** Вес сетки и надетого, кг. */
	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	float GetTotalWeight() const;

	const FRubezhInventoryGrid& GetGrid() const { return Grid; }

	const FRubezhItemDefinition* FindDefinition(FName ItemId) const;

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
	void OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	void OnCarriedWeightChanged(const FOnAttributeChangeData& ChangeData);
	void RefreshOwnerWalkSpeed() const;

	void HandleGridChanged();
	void UpdateCarriedWeight() const;
	void ApplyStartingEquipment();
	void TrySyncWeightAttributes();
	void LogEquipFailure(Rubezh::Equipment::EResult Result) const;

	FDelegateHandle CarriedWeightChangedHandle;
	FTimerHandle WeightSyncTimer;

	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	FRubezhInventoryGrid Grid;

	// Надетое по слотам (порядок — Rubezh::Equipment::AllSlots).
	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	TArray<FName> Equipped;

	UPROPERTY(Transient)
	TObjectPtr<const UDataTable> ItemTable;
};
