// РУБЕЖ: Земля после разлома

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"

RUBEZHCORERUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogRubezh, Log, All);

class FRubezhCoreRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
