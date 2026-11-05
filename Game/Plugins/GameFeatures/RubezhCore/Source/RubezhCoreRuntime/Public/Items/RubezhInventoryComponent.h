// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/ActorComponent.h"
#include "Items/RubezhInventoryGrid.h"

#include "RubezhInventoryComponent.generated.h"

class APawn;
class APlayerState;
class UDataTable;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRubezhInventoryChangedEvent);

/**
 * Рюкзак игрока. Живёт на PlayerState, содержимое меняет только сервер и реплицирует владельцу.
 * После каждого изменения пересчитывает вес в атрибут CarriedWeight (URubezhSurvivalSet),
 * от которого зависят перегруз, скорость и спринт.
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

	UFUNCTION(BlueprintPure, Category = "Rubezh|Inventory")
	int32 CountItem(FName ItemId) const { return Grid.CountItem(ItemId); }

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

	FDelegateHandle CarriedWeightChangedHandle;

	UPROPERTY(ReplicatedUsing = OnRep_Grid)
	FRubezhInventoryGrid Grid;

	UPROPERTY(Transient)
	TObjectPtr<const UDataTable> ItemTable;
};
