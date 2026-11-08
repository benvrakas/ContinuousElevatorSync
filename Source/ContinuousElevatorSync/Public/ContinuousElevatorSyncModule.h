#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogContinuousElevatorSync, Log, All);

class FContinuousElevatorSyncModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool Tick(float DeltaTime);

	FTSTicker::FDelegateHandle TickHandle;
};
