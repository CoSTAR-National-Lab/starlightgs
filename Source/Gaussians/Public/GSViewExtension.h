

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
	//Rasterization Pipeline // rendering into scene texture to begin with as a post process. later can do
	// TODO: Will be combined into single function to avoid storing transient buffers as class members
	void GSPrePass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const TSet<FGaussianSplatSceneProxy*>& Proxies);
	void GSSplattingPass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, bool bRelighting);

	// Composite splat buffer with GBuffer
	void GSCompositionPass(FRDGBuilder& GraphBuilder, const FSceneView& InView);

public:
	void RegisterProxy(FGaussianSplatSceneProxy* Proxy);
	void UnregisterProxy(FGaussianSplatSceneProxy* Proxy);

private:
	// Relit and unlit splats are kept separate as they are rendered in different passes
	TSet<FGaussianSplatSceneProxy*> RelightableProxies;
	TSet<FGaussianSplatSceneProxy*> NonRelightableProxies;

	// TODO: Will remove transient resources from class members
	TMap<FName, FRDGBufferRef> TransientBuffers;
	FGSplatGBuffer GSplatGBuffer;

	UWorldSubsystem* OwningSubsystem;
};