// РУБЕЖ: Земля после разлома

#pragma once

#include "Engine/DataTable.h"

#include "RubezhWeaponDefinition.generated.h"

/** В какой слот берётся оружие. */
UENUM(BlueprintType)
enum class ERubezhWeaponClass : uint8
{
	Primary,	// автомат, дробовик, винтовка: два основных слота
	Sidearm		// пистолет: свой слот
};

/**
 * Боевые характеристики оружия — строка DataTable, импортируется из data/weapons.csv.
 * Имя строки — идентификатор предмета из data/items.csv (rifle_762x39, ...).
 * ShotsPerMinute, bSemiAuto и Pellets применяются к ассетам Lyra скриптом tools/unreal/setup_weapons.py.
 */
USTRUCT(BlueprintType)
struct RUBEZHCORERUNTIME_API FRubezhWeaponDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon")
	ERubezhWeaponClass WeaponClass = ERubezhWeaponClass::Primary;

	// Какими патронами стреляет.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon")
	FName AmmoItem;

	// Какой магазин подходит. None — магазина нет, патроны заряжаются прямо из рюкзака (дробовик).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon")
	FName MagazineItem;

	// Сколько патронов вмещает оружие без магазина.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon", Meta = (ClampMin = 0))
	int32 TubeCapacity = 0;

	// Урон одной пули или дробины.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon", Meta = (ClampMin = 0))
	int32 Damage = 0;

	// Сколько дробин в одном патроне.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon", Meta = (ClampMin = 1))
	int32 Pellets = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon", Meta = (ClampMin = 1))
	float ShotsPerMinute = 60.0f;

	// Один выстрел на нажатие (иначе стреляет, пока кнопка зажата).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon")
	bool bSemiAuto = true;

	// В этих пределах урон полный, дальше падает (см. Rubezh::Weapons::GetDistanceMultiplier).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon", Meta = (ClampMin = 1))
	float EffectiveRangeM = 100.0f;

	// На сколько градусов выстрел поднимает прицел.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon|Recoil", Meta = (ClampMin = 0))
	float RecoilPitch = 0.0f;

	// Наибольший случайный увод прицела вбок, градусов.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon|Recoil", Meta = (ClampMin = 0))
	float RecoilYaw = 0.0f;

	// Доля, на которую отдача растёт с каждым следующим выстрелом очереди.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon|Recoil", Meta = (ClampMin = 0))
	float RecoilGrowth = 0.0f;

	// Предмет Lyra (ID_*), который даёт модель и способности стрельбы.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubezh|Weapon")
	TSoftClassPtr<UObject> LyraItem;
};
