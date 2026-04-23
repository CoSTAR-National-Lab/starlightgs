#include "GSViewExtension.h"

#include "GaussianSplatSceneProxy.h"
#include "GaussianSplatResource.h"
#include "RenderGraphUtils.h"
#include "PostProcess/PostProcessInputs.h"

#include "Subsystems/WorldSubsystem.h"


FGSViewExtension::FGSViewExtension(const FAutoRegister& AutoRegister, UWorldSubsystem* Owner) : FSceneViewExtensionBase(AutoRegister), OwningSubsystem(Owner)
{
}

void FGSViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{

}

void FGSViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	
}

void FGSViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FGSViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
	const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	if (Proxies.IsEmpty())
	{
		return;
	}

	GSplatGBuffer.Init(GraphBuilder, InView);

	GSplatGBuffer.Clear(GraphBuilder);
	

	GSPrePass(GraphBuilder, InView);
	GSSplattingPass(GraphBuilder, InView);
	
}

void FGSViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
	const FPostProcessingInputs& Inputs)
{
	if (Proxies.IsEmpty())
	{
		return;
	}

	//after splattery copy back to scene 
	AddCopyTexturePass(GraphBuilder, GSplatGBuffer.GetBaseColorTexture(), Inputs.SceneTextures->GetContents()->SceneColorTexture);
}

bool FGSViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	//check if main viewport is valid and if the world of the context match the world of the owning subsystem
	return Context.GetWorld() == OwningSubsystem->GetWorld();
}

void FGSViewExtension::RegisterProxy(class FGaussianSplatSceneProxy* Proxy)
{
	check(!Proxies.Contains(Proxy));
	Proxies.Add(Proxy);
}

void FGSViewExtension::UnregisterProxy(class FGaussianSplatSceneProxy* Proxy)
{
	check(Proxies.Contains(Proxy));
	Proxies.Remove(Proxy);
}
