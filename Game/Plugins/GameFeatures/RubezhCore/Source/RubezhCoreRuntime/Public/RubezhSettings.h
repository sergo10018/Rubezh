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

	// Размер сетки рюкзака в клетках, пока рюкзак не выбирается снаряжением.
	UPROPERTY(Config, EditAnywhere, Category = "Items", Meta = (ClampMin = 1))
	FIntPoint DefaultBackpackSize = FIntPoint(6, 8);

	// Клавиша, открывающая и закрывающая окно рюкзака.
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InventoryKey = EKeys::I;

	// Клавиша взаимодействия: подобрать предмет, на который смотришь.
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	FKey InteractKey = EKeys::E;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
