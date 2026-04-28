#include "GSViewExtension.h"

#include "GaussianSplatSceneProxy.h"
#include "GaussianSplatResource.h"
#include "RenderGraphUtils.h"
#include "PostProcess/PostProcessInputs.h"

#include "Subsystems/WorldSubsystem.h"

DEFINE_STAT(STAT_SplatsRendered)

FGSViewExtension::FGSViewExtension(const FAutoRegister& AutoRegister, UWorldSubsystem* Owner) : FSceneViewExtensionBase(AutoRegister), OwningSubsystem(Owner)
{
}

void FGSViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	// TODO: Won't work if multiple views active simultaneously
	SET_DWORD_STAT(STAT_SplatsRendered, 0)
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
	RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Lit");

	if (RelightableProxies.IsEmpty())
	{
		return;
	}

	GSplatGBuffer.Init(GraphBuilder, InView);

	// RenderGraph doesn't recreate the resource every frame, so we need to clear it manually
	GSplatGBuffer.Clear(GraphBuilder);
	
	GSPrePass(GraphBuilder, InView, RelightableProxies);
	// TODO: Once GSPrePass and GSSplattingPass are combined, bRelighting can be deduced from proxies passed in
	GSSplattingPass(GraphBuilder, InView, GSplatGBuffer.GetRenderTargetBindings(), true);

	GSCompositionPass(GraphBuilder, InView);
}

void FGSViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
	const FPostProcessingInputs& Inputs)
{
	// Render all splats that don't support relighting
	RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Unlit");

	if (!NonRelightableProxies.IsEmpty())
	{
		GSPrePass(GraphBuilder, InView, NonRelightableProxies);

		FRenderTargetBindingSlots RenderTargets{};
		RenderTargets[0] = FRenderTargetBinding{ Inputs.SceneTextures->GetContents()->SceneColorTexture, ERenderTargetLoadAction::ELoad };
		// TODO: Once GSPrePass and GSSplattingPass are combined, bRelighting can be deduced from proxies passed in
		GSSplattingPass(GraphBuilder, InView, RenderTargets, false);
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
