#include "GaussiansEditorModule.h"

#include "GaussianSplatComponent.h"
#include "Modules/ModuleManager.h"


DEFINE_LOG_CATEGORY(LogGSplatEditor);

IMPLEMENT_MODULE(FGaussiansEditor, GaussiansEditor);

void FGaussiansEditor::StartupModule()
{
	GSplatAssetTypeActions = MakeShared<FGSplatAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());

	FrostingAssetTypeActions = MakeShared<FFrostingAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(FrostingAssetTypeActions.ToSharedRef());

	ComponentAssetBroker = MakeShared<FGSplatCommponentAssetBroker>();
	FComponentAssetBrokerage::RegisterBroker(ComponentAssetBroker, UGaussianSplatComponent::StaticClass(), true, true);
}

void FGaussiansEditor::ShutdownModule()
{
	if (FAssetToolsModule* AssetTools = FModuleManager::Get().GetModulePtr<FAssetToolsModule>("AssetTools"))
	{
		AssetTools->Get().UnregisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());
		AssetTools->Get().UnregisterAssetTypeActions(FrostingAssetTypeActions.ToSharedRef());
	}

	if (ComponentAssetBroker.IsValid())
	{
		FComponentAssetBrokerage::UnregisterBroker(ComponentAssetBroker);
	}
}

