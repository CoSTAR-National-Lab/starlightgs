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
	// Process relightable proxies

	if (RelightableProxies.IsEmpty())
	{
		return;
	}

	GSplatGBuffer.Init(GraphBuilder, InView);

	//RenderGraph doesnt recreate the resource every frame, so we need to clear it manually
	GSplatGBuffer.Clear(GraphBuilder);
	

	GSPrePass(GraphBuilder, InView, RelightableProxies);
	GSSplattingPass(GraphBuilder, InView);
}

void FGSViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
	const FPostProcessingInputs& Inputs)
{
	// Render all splats that don't support relighting
	if (!NonRelightableProxies.IsEmpty())
	{
		// TODO: Require different permutation of CS/VS and different pixel shader
		// TODO: As well as different render target.
	}

	// TODO: Will be replaced by GSplatGBuffer composition pass (which will occur post-base pass)
	if (!RelightableProxies.IsEmpty())
	{
		//after splattery copy back to scene 
		AddCopyTexturePass(GraphBuilder, GSplatGBuffer.GetBaseColorTexture(), Inputs.SceneTextures->GetContents()->SceneColorTexture);
	}
}

bool FGSViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	//check if main viewport is valid and if the world of the context match the world of the owning subsystem
	return Context.GetWorld() == OwningSubsystem->GetWorld();
}

void FGSViewExtension::RegisterProxy(class FGaussianSplatSceneProxy* Proxy)
{
	if (Proxy->SupportsRelighting())
	{
		check(!RelightableProxies.Contains(Proxy));
		RelightableProxies.Add(Proxy);
	}
	else
	{
		check(!NonRelightableProxies.Contains(Proxy));
		NonRelightableProxies.Add(Proxy);
	}
}

void FGSViewExtension::UnregisterProxy(class FGaussianSplatSceneProxy* Proxy)
{
	if (Proxy->SupportsRelighting())
	{
		check(RelightableProxies.Contains(Proxy));
		RelightableProxies.Remove(Proxy);
	}
	else
	{
		check(NonRelightableProxies.Contains(Proxy));
		NonRelightableProxies.Remove(Proxy);
	}
}
