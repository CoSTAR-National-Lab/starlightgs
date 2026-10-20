#include "GaussiansEditorModule.h"

#include "Modules/ModuleManager.h"
#include "Builders/CubeBuilder.h"
#include "Builders/TetrahedronBuilder.h"

#include "GaussianSplatComponent.h"

#include "GaussianSplatClippingVolume.h"
#include "GSClippingVolumeRebuildService.h"
#include "GSplatExporter.h"


DEFINE_LOG_CATEGORY(LogGSplatEditor);

IMPLEMENT_MODULE(FGaussiansEditor, GaussiansEditor);

void FGaussiansEditor::StartupModule()
{
	GSplatAssetTypeActions = MakeShared<FGSplatAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());

	ComponentAssetBroker = MakeShared<FGSplatCommponentAssetBroker>();
	FComponentAssetBrokerage::RegisterBroker(ComponentAssetBroker, UGaussianSplatComponent::StaticClass(), true, true);

	FGSClippingVolumeRebuildService::GetOnNeedsRebuild().AddStatic(&FGaussiansEditor::OnClippingVolumeNeedsBrushRebuild);

	ToolMenuStartupCallbackHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FGaussianSplatDataExtension::RegisterMenus));
}

void FGaussiansEditor::ShutdownModule()
{
	if (FAssetToolsModule* AssetTools = FModuleManager::Get().GetModulePtr<FAssetToolsModule>("AssetTools"))
	{
		AssetTools->Get().UnregisterAssetTypeActions(GSplatAssetTypeActions.ToSharedRef());
	}

	if (ComponentAssetBroker.IsValid())
	{
		FComponentAssetBrokerage::UnregisterBroker(ComponentAssetBroker);
	}

	FGSClippingVolumeRebuildService::GetOnNeedsRebuild().RemoveAll(this);

	UToolMenus::UnRegisterStartupCallback(ToolMenuStartupCallbackHandle);
}

void FGaussiansEditor::OnClippingVolumeNeedsBrushRebuild(AGaussianSplatClippingVolume* ClippingVolume)
{
	if (ClippingVolume->Shape == EGaussianSplatClippingVolumeShape::Box)
	{
		UCubeBuilder* CubeBuilder = Cast<UCubeBuilder>(DuplicateObject(GEditor->FindBrushBuilder(UCubeBuilder::StaticClass()), ClippingVolume));

		CubeBuilder->X = 200;
		CubeBuilder->Y = 200;
		CubeBuilder->Z = 200;
		ClippingVolume->BrushBuilder = CubeBuilder;
	}
	else if (ClippingVolume->Shape == EGaussianSplatClippingVolumeShape::Sphere)
	{
		UTetrahedronBuilder* SphereBuilder = Cast<UTetrahedronBuilder>(DuplicateObject(GEditor->FindBrushBuilder(UTetrahedronBuilder::StaticClass()), ClippingVolume));

		SphereBuilder->Radius = 100;
		SphereBuilder->SphereExtrapolation = 4;
		ClippingVolume->BrushBuilder = SphereBuilder;
	}
	else
	{
		checkf(false, TEXT("Forgot to implement new shape!"));
	}

	ClippingVolume->BrushBuilder->Build(ClippingVolume->GetWorld(), ClippingVolume);
	GEditor->RebuildAlteredBSP();
}
