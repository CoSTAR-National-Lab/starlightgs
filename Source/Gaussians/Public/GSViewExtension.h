#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

#include "GaussianSplatClippingVolume.h"
#include "Rendering/GSplatGBuffer.h"


DECLARE_STATS_GROUP(TEXT("Gaussians"), STATGROUP_Gaussians, STATCAT_Advanced);

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Splats Submitted: Lit"), STAT_SplatsSubmittedLit, STATGROUP_Gaussians, GAUSSIANS_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Splats Submitted: Unlit"), STAT_SplatsSubmittedUnlit, STATGROUP_Gaussians, GAUSSIANS_API);

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Splats Rendered: Lit"), STAT_SplatsRenderedLit, STATGROUP_Gaussians, GAUSSIANS_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Splats Rendered: Unlit"), STAT_SplatsRenderedUnlit, STATGROUP_Gaussians, GAUSSIANS_API);


class UGSWorldSubsystem;

class FGaussianSplatSceneProxy;
class FGSGPUStats;

/**
 * 
 */
class GAUSSIANS_API FGSViewExtension : public FSceneViewExtensionBase
{
public:
	FGSViewExtension(const FAutoRegister& AutoRegister, UGSWorldSubsystem* Owner);

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
	// Renders all splats in given proxies into supplied render targets.
	// Will determine from the proxies if relighting is supported (all proxies must have the same support for relighting)
	// If relighting is supported, supply a GSplatGBuffer to render into. Otherwise, supply the scene depth stencil
	static void GaussianSplattingPass(
		FRDGBuilder& GraphBuilder, 
		const FSceneView& InView, 
		const FRenderTargetBindingSlots& RenderTargets, 
		const TSet<FGaussianSplatSceneProxy*>& Proxies,
		const TArray<FGSClippingVolumeParameters>& ClippingVolumes,
		const FGSGPUStats* GPUStats
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

	UGSWorldSubsystem* OwningSubsystem = nullptr;

	TArray<FGSClippingVolumeParameters> ClippingVolumeParameters;

	TPimplPtr<FGSGPUStats> GPUStats = nullptr;
};
