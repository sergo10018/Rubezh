// РУБЕЖ: Земля после разлома

#include "Items/RubezhItemCatalog.h"

#include "Engine/DataTable.h"
#include "Items/RubezhItemDefinition.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"

namespace Rubezh::Items
{
	const FRubezhItemDefinition* FindDefinition(FName ItemId)
	{
		const UDataTable* Table = GetDefault<URubezhSettings>()->ItemTable.LoadSynchronous();
		return (Table && !ItemId.IsNone()) ? Table->FindRow<FRubezhItemDefinition>(ItemId, TEXT("RubezhItemCatalog"), false) : nullptr;
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
