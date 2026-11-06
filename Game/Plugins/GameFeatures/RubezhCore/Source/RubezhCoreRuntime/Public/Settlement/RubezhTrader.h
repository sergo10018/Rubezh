// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"
#include "Items/RubezhTraderKind.h"

#include "RubezhTrader.generated.h"

class UStaticMeshComponent;

/**
 * Торговец в поселении: ставится на карту, игрок подходит и нажимает клавишу взаимодействия.
 * Сам ничего не решает: сделки считает сервер в URubezhInventoryComponent (RubezhInventoryComponent_Trade.cpp)
 * по столбцам Price и Trader из data/items.csv. Запас товара бесконечный.
 * Пока это цветной столб: модели и анимаций NPC в проекте нет.
 */
UCLASS(Blueprintable)
class RUBEZHCORERUNTIME_API ARubezhTrader : public AActor
{
	GENERATED_BODY()

public:
	ARubezhTrader(const FObjectInitializer& ObjectInitializer);

	ERubezhTraderKind GetKind() const { return Kind; }
	FText GetDisplayName() const;
	FVector GetFocusLocation() const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Kind();

	void UpdateVisual();

	// Кем работает этот торговец.
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Kind, Category = "Rubezh|Trader")
	ERubezhTraderKind Kind = ERubezhTraderKind::Scavenger;

	UPROPERTY(VisibleAnywhere, Category = "Rubezh|Trader")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
