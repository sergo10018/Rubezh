// РУБЕЖ: Земля после разлома

#include "RubezhCoreRuntimeModule.h"

#include "AbilitySystem/Executions/LyraDamageExecution.h"
#include "Items/RubezhInventoryComponent.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRubezh);

void FRubezhCoreRuntimeModule::StartupModule()
{
	UE_LOG(LogRubezh, Log, TEXT("RubezhCoreRuntime module started"));
	// Броня и одежда уменьшают урон от попаданий: расчёт урона Lyra спрашивает множитель у снаряжения цели.
	ULyraDamageExecution::GetIncomingDamageMultiplier().BindStatic(&URubezhInventoryComponent::GetIncomingDamageMultiplier);
}

void FRubezhCoreRuntimeModule::ShutdownModule()
{
	ULyraDamageExecution::GetIncomingDamageMultiplier().Unbind();
}

IMPLEMENT_MODULE(FRubezhCoreRuntimeModule, RubezhCoreRuntime)
