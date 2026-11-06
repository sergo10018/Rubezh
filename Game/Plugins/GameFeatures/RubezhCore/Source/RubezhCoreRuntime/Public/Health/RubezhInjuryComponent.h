// РУБЕЖ: Земля после разлома

#pragma once

#include "Components/PawnComponent.h"
#include "Health/RubezhInjury.h"
#include "Math/RandomStream.h"

#include "RubezhInjuryComponent.generated.h"

class UAbilitySystemComponent;
class URubezhHealthStatusWidget;
struct FGameplayEffectSpec;
struct FRubezhItemDefinition;

/** Отложенная часть лечения аптечкой: добавится в указанное время. */
USTRUCT()
struct FRubezhPendingHeal
{
	GENERATED_BODY()

	UPROPERTY()
	double At = 0.0;

	UPROPERTY()
	float Percent = 0.0f;
};

/** Травмы и текущее лечение. Время — серверное (GameState::GetServerWorldTimeSeconds), чтобы клиент мог показать прогресс. */
USTRUCT()
struct FRubezhInjuryState
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Injuries = 0;

	// Боль проходит сама в это время.
	UPROPERTY()
	double PainUntil = 0.0;

	// Обезболивающее действует до этого времени.
	UPROPERTY()
	double PainReliefUntil = 0.0;

	// Что сейчас применяется (None — лечения нет).
	UPROPERTY()
	FName TreatmentItem;

	UPROPERTY()
	double TreatmentStart = 0.0;

	UPROPERTY()
	double TreatmentEnd = 0.0;

	// Аптечки, которые ещё действуют (вторая часть здоровья придёт позже).
	UPROPERTY()
	TArray<FRubezhPendingHeal> PendingHeals;
};

/**
 * Травмы и лечение (docs/gdd/01_Системы_прототипа.md §3). Живёт на персонаже (добавляет URubezhComponentInjector).
 * Сервер: выдаёт травмы при попадании и падении, отнимает здоровье при кровотечении, проводит лечение
 * (предмет расходуется в конце). Свой клиент: дрожание прицела от боли и строка состояния на экране.
 * Травмы дублируются тегами Status.Injury.* на ASC — по ним, например, спринт не работает при переломе.
 */
UCLASS(ClassGroup = Rubezh, Meta = (BlueprintSpawnableComponent))
class RUBEZHCORERUNTIME_API URubezhInjuryComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	URubezhInjuryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static URubezhInjuryComponent* FindForActor(const AActor* Actor);

	/** Применить предмет из рюкзака (двойной клик в окне). Сервер проверит, есть ли предмет и нужен ли он. */
	void UseItem(FName ItemId);

	/** Быстрое лечение (клавиша H): выбрать самое нужное из рюкзака и применить. */
	void QuickTreat();

	/** Только сервер: нанести травмы (попадание, падение, консольная команда). */
	void AddInjuries(Rubezh::Injuries::FMask NewInjuries);

	const FRubezhInjuryState& GetState() const { return State; }
	bool IsTreating() const { return !State.TreatmentItem.IsNone(); }
	bool IsPainRelieved() const;
	float GetTreatmentProgress() const;

	/** Сколько секунд осталось до ближайшей отложенной части лечения (0 — ничего не ждём). */
	float GetPendingHealSecondsLeft() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerUseItem(FName ItemId);

	UFUNCTION()
	void OnRep_State();

	// --- сервер ---
	void TickServer(float DeltaTime);
	void TryBindHealth();
	void HandleHealthChanged(AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
	void TrackFall();
	void ApplyBleeding(float DeltaTime);
	void UpdatePain();
	void ApplyPendingHeals();
	void HealPercent(float Percent) const;
	void FinishTreatment();
	void ApplyHealthEffect(TSubclassOf<class UGameplayEffect> EffectClass, const FGameplayTag& MagnitudeTag, float Amount) const;
	void SyncInjuryTags() const;
	void ClearInjuryTags() const;

	// --- все ---
	void ApplyTreatmentBlock(bool bBlock);
	void CancelBlockedAbilities() const;

	// --- свой клиент ---
	void TickLocal(float DeltaTime);
	void ApplyPainSway(float DeltaTime);
	void UpdateStatusWidget();

	UAbilitySystemComponent* GetAbilitySystem() const;
	double GetServerTime() const;
	bool IsDead() const;
	bool IsLocalPlayerPawn() const;
	void CollectInventoryMedicine(TArray<TPair<FName, const FRubezhItemDefinition*>>& OutAvailable) const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FRubezhInjuryState State;

	UPROPERTY(Transient)
	TObjectPtr<URubezhHealthStatusWidget> StatusWidget;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
	FDelegateHandle HealthChangedHandle;
	FRandomStream Random;

	float BleedAccumulator = 0.0f;
	float FallApexZ = 0.0f;
	bool bWasFalling = false;
	bool bAbilitiesBlocked = false;
	// Какой слот оружия чистится (только сервер): игрок мог сменить оружие, пока идёт чистка.
	int32 CleaningWeaponSlot = INDEX_NONE;
	FName CleaningWeaponItem;
	float PainSwayTime = 0.0f;
	FRotator PainSwayOffset = FRotator::ZeroRotator;
};
