#pragma once

#include "PrimitiveSceneProxy.h"


class UGaussianSplatComponent;
class FGaussianSplatResource;
class FGaussianSplatResourceWrapper;
class FGSViewExtension;


class FGaussianSplatSceneProxy : public FPrimitiveSceneProxy
{
public:
	FGaussianSplatSceneProxy(UGaussianSplatComponent* InComponent);
	virtual ~FGaussianSplatSceneProxy();

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
	virtual void DestroyRenderThreadResources() override;

#if WITH_EDITOR
	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy>>& OutHitProxies) override;
#endif

	virtual SIZE_T GetTypeHash() const override;
	virtual uint32 GetMemoryFootprint() const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views, 
		const FSceneViewFamily& ViewFamily, 
		uint32 VisibilityMap, 
		FMeshElementCollector& Collector) const override;
	//~ End FPrimitiveSceneProxy Interface

	FGaussianSplatResource* GetResource_RenderThread() const;

	bool SupportsRelighting() const;


	inline float GetScaleMultiplier() const { return ScaleMultiplier; }

private:
	// GPU Resources for the asset pointed to by the component
	// It is NOT safe to dereference this pointer on the game thread
	TSharedPtr<FGaussianSplatResourceWrapper> GaussianSplatResource_RenderThread;

	TWeakPtr<FGSViewExtension> ViewExtension;

#if WITH_EDITOR
	// Cached hit proxy created in CreateHitProxies, used for editor viewport click selection.
	HHitProxy* SelectionHitProxy = nullptr;
#endif

	// Per-instance properties
	float ScaleMultiplier = 1.0f;
};
