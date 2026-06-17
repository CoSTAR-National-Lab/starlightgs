#pragma once

#include "GaussianSplatVertexFactory.h"
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
	inline int32 GetClippingVolumeMask() const { return ClippingVolumeMask; }

private:
	// GPU Resources for the asset pointed to by the component
	// It is NOT safe to dereference this pointer on the game thread
	TSharedPtr<FGaussianSplatResourceWrapper> GaussianSplatResource_RenderThread;

	TWeakPtr<FGSViewExtension> ViewExtension;

	// Per-instance properties
	float ScaleMultiplier = 1.0f;
	int32 ClippingVolumeMask = 0;
	bool bDrawVFInMainPass = false;

	//Proxy vertex factory
	FGaussianSplatVertexFactory VertexFactory;
	FMaterialRenderProxy* ShadowMaterialProxy;
};
