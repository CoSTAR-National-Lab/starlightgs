#include "GSViewExtension.h"

#include "GaussianSplatSceneProxy.h"
#include "Rendering/GSGPUStats.h"

#include "RenderGraphUtils.h"
#include "PostProcess/PostProcessInputs.h"

#include "Subsystems/WorldSubsystem.h"

DEFINE_STAT(STAT_SplatsSubmittedLit)
DEFINE_STAT(STAT_SplatsSubmittedUnlit)
DEFINE_STAT(STAT_SplatsRenderedLit)
DEFINE_STAT(STAT_SplatsRenderedUnlit)


FGSViewExtension::FGSViewExtension(const FAutoRegister& AutoRegister, UWorldSubsystem* Owner) : FSceneViewExtensionBase(AutoRegister), OwningSubsystem(Owner)
{
	GPUStats = MakePimpl<FGSGPUStats>();
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

void FGSViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	GPUStats->ProcessBeginRender(GraphBuilder);
}

void FGSViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
                                                               const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	// Process relightable proxies
	if (!RelightableProxies.IsEmpty())
	{
		RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Lit");

		FGSplatGBuffer GSplatGBuffer;
		GSplatGBuffer.Init(GraphBuilder, InView);

		{
			RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Clear GSplatGBuffer");
			GSplatGBuffer.Clear(GraphBuilder);
		}

		GaussianSplattingPass(GraphBuilder, InView, GSplatGBuffer.GetRenderTargetBindings(), RelightableProxies, GPUStats.Get());
		GSCompositionPass(GraphBuilder, InView, GSplatGBuffer);
	}
}

void FGSViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
	const FPostProcessingInputs& Inputs)
{
	// Render all splats that don't support relighting
	if (!NonRelightableProxies.IsEmpty())
	{
		RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Unlit");
		const FSceneTextureUniformParameters* SceneTextures = Inputs.SceneTextures->GetContents();
		FRDGTextureRef SceneColorTexture = SceneTextures->SceneColorTexture;
		FRDGTextureRef SceneDepthTexture = SceneTextures->SceneDepthTexture;

		FRenderTargetBindingSlots RenderTargets{};
		RenderTargets[0] = FRenderTargetBinding{ SceneColorTexture, ERenderTargetLoadAction::ELoad };
		RenderTargets.DepthStencil = FDepthStencilBinding{
			SceneDepthTexture, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthRead_StencilNop
		};

		GaussianSplattingPass(GraphBuilder, InView, RenderTargets, NonRelightableProxies, GPUStats.Get());
	}
}

void FGSViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	GPUStats->ProcessEndRender(GraphBuilder);
}

bool FGSViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	//check if main viewport is valid and if the world of the context match the world of the owning subsystem
	return Context.GetWorld() == OwningSubsystem->GetWorld();
}

void FGSViewExtension::RegisterProxy(FGaussianSplatSceneProxy* Proxy)
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

void FGSViewExtension::UnregisterProxy(FGaussianSplatSceneProxy* Proxy)
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
