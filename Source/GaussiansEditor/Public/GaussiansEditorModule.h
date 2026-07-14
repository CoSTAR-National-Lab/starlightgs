#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#include "GSplatAssetTypeActions.h"
#include "FrostingAssetTypeActions.h"
#include "GSplatCommponentAssetBroker.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGSplatEditor, Log, All)

class FGaussiansEditor : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	TSharedPtr<FGSplatAssetTypeActions> GSplatAssetTypeActions;
	TSharedPtr<FFrostingAssetTypeActions> FrostingAssetTypeActions;
	TSharedPtr<FGSplatCommponentAssetBroker> ComponentAssetBroker;

};
