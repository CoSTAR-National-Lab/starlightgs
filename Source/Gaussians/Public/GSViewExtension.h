

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

#include "Rendering/GSplatGBuffer.h"

DECLARE_STATS_GROUP(TEXT("Gaussians"), STATGROUP_Gaussians, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("SplatsRendered"), STAT_SplatsRendered, STATGROUP_Gaussians, GAUSSIANS_API);

class UWorldSubsystem;
class FGaussianSplatSceneProxy;

/**
 * 
 */

class GAUSSIANS_API FGSViewExtension : public FSceneViewExtensionBase
{
public:
	FGSViewExtension(const FAutoRegister& AutoRegister, UWorldSubsystem* Owner);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
	// Renders all splats in given proxies into supplied render targets.
	// Will determine from the proxies if relighting is supported (all proxies must have the same support for relighting)
	// If relighting is supported, supply a GSplatGBuffer to render into. Otherwise, supply the scene depth stencil
	static void GaussianSplattingPass(
		FRDGBuilder& GraphBuilder, 
		const FSceneView& InView, 
		const FRenderTargetBindingSlots& RenderTargets, 
		const TSet<FGaussianSplatSceneProxy*>& Proxies
	);

	// Composite splat buffer with GBuffer
	static void GSCompositionPass(
		FRDGBuilder& GraphBuilder, 
		const FSceneView& InView, 
		const FGSplatGBuffer& GSplatGBuffer
	);

public:
	void RegisterProxy(FGaussianSplatSceneProxy* Proxy);
	void UnregisterProxy(FGaussianSplatSceneProxy* Proxy);

private:
	// Relit and unlit splats are kept separate as they are rendered in different passes
	TSet<FGaussianSplatSceneProxy*> RelightableProxies;
	TSet<FGaussianSplatSceneProxy*> NonRelightableProxies;

	UWorldSubsystem* OwningSubsystem;
};