// РУБЕЖ: Земля после разлома

#include "Items/RubezhItemCatalog.h"

#include "Animals/RubezhAnimalRules.h"
#include "Engine/DataTable.h"
#include "Items/RubezhItemDefinition.h"
#include "Items/RubezhWeaponDefinition.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"

namespace Rubezh::Items
{
	const FRubezhItemDefinition* FindDefinition(FName ItemId)
	{
		const UDataTable* Table = GetDefault<URubezhSettings>()->ItemTable.LoadSynchronous();
		return (Table && !ItemId.IsNone()) ? Table->FindRow<FRubezhItemDefinition>(ItemId, TEXT("RubezhItemCatalog"), false) : nullptr;
	}

	const FRubezhWeaponDefinition* FindWeapon(FName ItemId)
	{
		const UDataTable* Table = GetDefault<URubezhSettings>()->WeaponTable.LoadSynchronous();
		return (Table && !ItemId.IsNone()) ? Table->FindRow<FRubezhWeaponDefinition>(ItemId, TEXT("RubezhItemCatalog"), false) : nullptr;
	}

	const FRubezhAnimalDefinition* FindAnimal(FName AnimalId)
	{
		const UDataTable* Table = GetDefault<URubezhSettings>()->AnimalTable.LoadSynchronous();
		return (Table && !AnimalId.IsNone()) ? Table->FindRow<FRubezhAnimalDefinition>(AnimalId, TEXT("RubezhItemCatalog"), false) : nullptr;
	}

	TArray<FName> GetItemIds()
	{
		const UDataTable* Table = GetDefault<URubezhSettings>()->ItemTable.LoadSynchronous();
		return Table ? Table->GetRowNames() : TArray<FName>();
	}

	TArray<FRubezhLootTableRow> GetLootRows()
	{
		TArray<FRubezhLootTableRow> Rows;
		const UDataTable* Table = GetDefault<URubezhSettings>()->LootTable.LoadSynchronous();
		if (!Table)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh: не задана таблица лута (Project Settings → Game → Rubezh → Loot Table)"));
			return Rows;
		}

		Table->ForeachRow<FRubezhLootTableRow>(TEXT("RubezhItemCatalog"), [&Rows](const FName& Key, const FRubezhLootTableRow& Row)
		{
			Rows.Add(Row);
		});
		return Rows;
	}
}
