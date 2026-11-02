// РУБЕЖ: Земля после разлома

#include "RubezhCoreRuntimeModule.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRubezh);

void FRubezhCoreRuntimeModule::StartupModule()
{
	UE_LOG(LogRubezh, Log, TEXT("RubezhCoreRuntime module started"));
}

void FRubezhCoreRuntimeModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FRubezhCoreRuntimeModule, RubezhCoreRuntime)
