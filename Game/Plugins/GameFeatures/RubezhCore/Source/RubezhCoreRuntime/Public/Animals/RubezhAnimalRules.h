// РУБЕЖ: Земля после разлома
// Животные (канон §3.1: только реальные животные): что делает зверь и когда стая бежит. Только данные и математика — проверяется тестами.
// Числа каждого вида — data/animals.csv (docs/gdd/01_Системы_прототипа.md §4).

#pragma once

#include "Engine/DataTable.h"

#include "RubezhAnimalRules.generated.h"

class UAnimationAsset;
class USkeletalMesh;

UENUM(BlueprintType)
enum class ERubezhAnimalState : uint8
{
	Wander,		// бродит у логова
	Chase,		// бежит к цели
	Bite,		// цель рядом — кусает
	Flee,		// убегает
	Dead
};

/** Вид животного: строка таблицы /RubezhCore/Data/DT_Animals. */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhAnimalDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal")
	float MaxHealth = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Attack")
	float BiteDamage = 10.0f;

	// Сколько секунд между укусами.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Attack")
	float BiteSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Attack")
	float BiteRangeCm = 150.0f;

	// Шанс укуса в ногу, проценты; остальное — в корпус. Нога может дать перелом.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Attack")
	float LegBitePercent = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Move")
	float WalkSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Move")
	float RunSpeed = 600.0f;

	// Таран: с этого расстояния до цели бежит со скоростью ChargeSpeed. 0 — не таранит.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Move")
	float ChargeStartCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Move")
	float ChargeSpeed = 0.0f;

	// С какого расстояния замечает человека.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Senses")
	float AggroRadiusCm = 3000.0f;

	// На каком расстоянии теряет цель.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Senses")
	float LoseRadiusCm = 6000.0f;

	// Убегает, когда здоровья осталось столько процентов. 0 — бьётся до смерти.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Flee")
	float FleeHealthPercent = 0.0f;

	// Стая бежит, когда потеряла столько процентов зверей. 0 — не бежит.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Flee")
	float PackRoutPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	float CapsuleRadiusCm = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	float CapsuleHalfHeightCm = 50.0f;

	// Модель и анимации. Пусто — вместо модели коробка.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<UAnimationAsset> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<UAnimationAsset> WalkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<UAnimationAsset> RunAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<UAnimationAsset> BiteAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	TSoftObjectPtr<UAnimationAsset> DeathAnim;

	// Масштаб и поворот модели: у разных паков разные.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	float MeshScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animal|Body")
	float MeshYaw = -90.0f;
};

namespace Rubezh::Animals
{
	/** Сколько секунд зверь убегает, прежде чем снова думать о нападении. */
	inline constexpr float FleeSeconds = 8.0f;

	/** На сколько см от логова зверь бродит. */
	inline constexpr float WanderRadiusCm = 1500.0f;

	/** На каком расстоянии от цели звери стаи занимают места вокруг неё. */
	inline constexpr float SurroundRadiusCm = 400.0f;

	/** Что зверь знает о мире в момент решения. */
	struct FSenses
	{
		bool bHasTarget = false;
		float TargetDistanceCm = 0.0f;
		bool bTargetInSafeZone = false;
		float HealthFraction = 1.0f;
		bool bPackRouted = false;
		bool bStillFleeing = false;
	};

	RUBEZHCORERUNTIME_API ERubezhAnimalState Decide(const FRubezhAnimalDefinition& Animal, const FSenses& Senses);

	/** Разбита ли стая: потеряла не меньше PackRoutPercent своих зверей. Одиночки не считаются. */
	RUBEZHCORERUNTIME_API bool IsPackRouted(const FRubezhAnimalDefinition& Animal, int32 Alive, int32 Total);

	/** Куда встать вокруг цели, чтобы окружить: равные углы по числу зверей. */
	RUBEZHCORERUNTIME_API FVector2D GetSurroundOffset(int32 Index, int32 Count, float RadiusCm);

	/** Скорость при погоне: таран начинается вблизи цели. */
	RUBEZHCORERUNTIME_API float GetChaseSpeed(const FRubezhAnimalDefinition& Animal, float TargetDistanceCm);
}
