// РУБЕЖ: Земля после разлома

#pragma once

#include "GameFramework/Actor.h"

#include "RubezhAnimalSpawner.generated.h"

class ARubezhAnimal;

/** Логово: ставится на карту, порождает стаю или одиночку. Когда все звери погибли, через RespawnSeconds появляются новые. Только сервер. */
UCLASS()
class RUBEZHCORERUNTIME_API ARubezhAnimalSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARubezhAnimalSpawner(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	/** Только сервер (для проверки): появиться стаей в этой точке. */
	static ARubezhAnimalSpawner* SpawnAt(UWorld& World, const FVector& Location, FName AnimalId, int32 Count);

	int32 CountAlive() const;
	int32 GetPackSize() const { return Count; }
	void AlertPack(APawn* Target);
	void HandleMemberDied();

private:
	void SpawnPack();

	// Строка из data/animals.csv.
	UPROPERTY(EditAnywhere, Category = "Rubezh|Animal")
	FName AnimalId = TEXT("dog");

	UPROPERTY(EditAnywhere, Category = "Rubezh|Animal", Meta = (ClampMin = 1))
	int32 Count = 1;

	// Через сколько секунд после гибели всех зверей появятся новые. 0 — не появятся.
	UPROPERTY(EditAnywhere, Category = "Rubezh|Animal", Meta = (ClampMin = 0))
	float RespawnSeconds = 600.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARubezhAnimal>> Members;

	FTimerHandle RespawnTimer;
};
