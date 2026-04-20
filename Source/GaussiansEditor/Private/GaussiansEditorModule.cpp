#include "GaussiansEditorModule.h"
#include "Modules/ModuleManager.h"


DEFINE_LOG_CATEGORY(LogGSplatEditor);

IMPLEMENT_MODULE(FGaussiansEditor, GaussiansEditor);

void FGaussiansEditor::StartupModule()
{
	GSplatAssetTypeActions = MakeShared<FGSplatAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());

	FrostingAssetTypeActions = MakeShared<FFrostingAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(FrostingAssetTypeActions.ToSharedRef());
}

void FGaussiansEditor::ShutdownModule()
{
	if (FAssetToolsModule* AssetTools = FModuleManager::Get().GetModulePtr<FAssetToolsModule>("AssetTools"))
	{
		AssetTools->Get().UnregisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());
		AssetTools->Get().UnregisterAssetTypeActions(FrostingAssetTypeActions.ToSharedRef());
	}
}

