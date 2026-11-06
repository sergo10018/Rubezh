// РУБЕЖ: Земля после разлома

#pragma once

#include "AbilitySystemInterface.h"
#include "Animals/RubezhAnimalRules.h"
#include "GameFramework/Character.h"

#include "RubezhAnimal.generated.h"

class ARubezhAnimalSpawner;
class ULyraAbilitySystemComponent;
class ULyraHealthSet;
class UStaticMeshComponent;

/**
 * Животное: решает только сервер. Живёт вне системы персонажей Lyra, но принимает урон от её оружия
 * через собственный набор здоровья. Числа вида — data/animals.csv, правила — Animals/RubezhAnimalRules.h.
 */
UCLASS()
class RUBEZHCORERUNTIME_API ARubezhAnimal : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARubezhAnimal(const FObjectInitializer& ObjectInitializer);

	/** Только сервер, до FinishSpawning. */
	void Setup(FName InAnimalId, ARubezhAnimalSpawner* InSpawner, int32 InPackIndex);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ERubezhAnimalState GetState() const { return State; }
	bool IsAlive() const { return State != ERubezhAnimalState::Dead; }
	FName GetAnimalId() const { return AnimalId; }

	/** Только сервер: стая заметила цель. */
	void Alert(APawn* NewTarget);

	/** Только сервер: сколько здоровья осталось, от 0 до 1. */
	float GetHealthFraction() const;

private:
	void ApplyDefinition();
	void UpdateBrain();
	void UpdateAnimation();
	APawn* FindNearestTarget() const;
	bool CanSee(const APawn& Candidate) const;
	void MoveForState(ERubezhAnimalState NewState, float TargetDistance);
	void MoveTo(const FVector& Destination, float Speed);
	void Bite();
	void Die();
	void HandleOutOfHealth(AActor* DamageInstigator, AActor* Causer, const struct FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
	void HandleHealthChanged(AActor* DamageInstigator, AActor* Causer, const struct FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);

	UFUNCTION()
	void OnRep_AnimalId();

	UPROPERTY(ReplicatedUsing = OnRep_AnimalId)
	FName AnimalId;

	UPROPERTY(Replicated)
	ERubezhAnimalState State = ERubezhAnimalState::Wander;

	UPROPERTY(VisibleAnywhere, Category = "Rubezh|Animal")
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<const ULyraHealthSet> HealthSet;

	// Коробка, пока у вида нет модели.
	UPROPERTY(VisibleAnywhere, Category = "Rubezh|Animal")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> PlayingAnim;

	UPROPERTY(Transient)
	TObjectPtr<APawn> Target;

	TWeakObjectPtr<ARubezhAnimalSpawner> Spawner;
	FVector HomeLocation = FVector::ZeroVector;
	FRubezhAnimalDefinition Definition;
	int32 PackIndex = 0;
	float NextBiteTime = 0.0f;
	float FleeUntil = 0.0f;
	float NextWanderTime = 0.0f;
	bool bFledFromRout = false;

	FTimerHandle BrainTimer;
	FTimerHandle AnimationTimer;
};
