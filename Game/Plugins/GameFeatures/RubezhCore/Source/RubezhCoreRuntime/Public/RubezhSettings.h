// РУБЕЖ: Земля после разлома

#pragma once

#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"

#include "RubezhSettings.generated.h"

class UDataTable;

/** Настройки проекта «Рубеж» (Project Settings → Game → Rubezh), хранятся в Config/DefaultGame.ini. */
UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "Rubezh"))
class RUBEZHCORERUNTIME_API URubezhSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Таблица предметов (строки FRubezhItemDefinition), импортированная из data/items.csv.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (RequiredAssetDataTags = "RowStructure=/Script/RubezhCoreRuntime.RubezhItemDefinition"))
	TSoftObjectPtr<UDataTable> ItemTable;

	// Таблица лута ящиков (строки FRubezhLootTableRow), импортированная из data/loot_tables.csv.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (RequiredAssetDataTags = "RowStructure=/Script/RubezhCoreRuntime.RubezhLootTableRow"))
	TSoftObjectPtr<UDataTable> LootTable;

	// Таблица оружия (строки FRubezhWeaponDefinition), импортированная из data/weapons.csv.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (RequiredAssetDataTags = "RowStructure=/Script/RubezhCoreRuntime.RubezhWeaponDefinition"))
	TSoftObjectPtr<UDataTable> WeaponTable;

	// Сколько ячеек без рюкзака — карманы.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (ClampMin = 1))
	int32 PocketSlots = 4;

	// Сколько лежат на земле вещи, выпавшие при смерти (docs/gdd/00_Core_loop.md §4), секунд.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (ClampMin = 0))
	float BodyLifetimeSeconds = 900.0f;

	// Что надето на новом персонаже (идентификаторы предметов из таблицы).
	UPROPERTY(Config, EditAnywhere, Category = "Items")
	TArray<FName> StartingEquipment = { TEXT("backpack_sack"), TEXT("jacket_windbreaker"), TEXT("pants_work") };

	// Оружие нового персонажа: заряжено, берётся в слоты.
	UPROPERTY(Config, EditAnywhere, Category = "Items")
	TArray<FName> StartingWeapons = { TEXT("pistol_9x18") };

	// Сколько полных запасных магазинов к стартовому оружию лежит в рюкзаке.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (ClampMin = 0))
	int32 StartingSpareMagazines = 1;

	// Сколько билетов у нового персонажа.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (ClampMin = 0))
	int32 StartingMoney = 200;

	// Клавиша, открывающая и закрывающая окно рюкзака.
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InventoryKey = EKeys::I;

	// Клавиша взаимодействия: подобрать предмет, на который смотришь.
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InteractKey = EKeys::E;

	// Быстрое лечение: применить самое нужное из рюкзака (жгут, бинт, шина, обезболивающее, аптечка).
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey HealKey = EKeys::H;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
