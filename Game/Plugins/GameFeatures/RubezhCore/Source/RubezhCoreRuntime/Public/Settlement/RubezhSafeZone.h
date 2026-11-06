// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"

#include "RubezhSafeZone.generated.h"

class AController;
class ALyraPlayerStart;
class UBoxComponent;

/**
 * Безопасная зона поселения: ставится на карту, размер — HalfSizeCm. Внутри оружие не стреляет
 * (URubezhSafeZoneComponent) и никто не получает урон от попаданий (URubezhInventoryComponent::GetIncomingDamageMultiplier).
 * Кровотечение внутри продолжается: его надо лечить. Вход только через КПП обеспечивают стены на карте.
 */
UCLASS(Blueprintable)
class RUBEZHCORERUNTIME_API ARubezhSafeZone : public AActor
{
	GENERATED_BODY()

public:
	ARubezhSafeZone(const FObjectInitializer& ObjectInitializer);

	/** Лежит ли точка в какой-нибудь безопасной зоне этого мира. */
	static bool IsInsideAny(const UWorld* World, const FVector& Location);

	bool Contains(const FVector& Location) const;

	/** Для выбора точки появления Lyra: игроки появляются в поселении, боты снаружи, иначе они не смогут стрелять. */
	static void FilterPlayerStarts(AController* Player, TArray<ALyraPlayerStart*>& PlayerStarts);

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// Половина размера зоны по каждой оси, см.
	UPROPERTY(EditAnywhere, Category = "Rubezh|SafeZone", Meta = (ClampMin = 0))
	FVector HalfSizeCm = FVector(3000.0, 3000.0, 1000.0);

	UPROPERTY(VisibleAnywhere, Category = "Rubezh|SafeZone")
	TObjectPtr<UBoxComponent> Box;
};
