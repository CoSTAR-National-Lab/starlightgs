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

	virtual SIZE_T GetTypeHash() const override;
	virtual uint32 GetMemoryFootprint() const override;
	//~ End FPrimitiveSceneProxy Interface

	FGaussianSplatResource* GetResource_RenderThread() const;

	bool SupportsRelighting() const;

private:
	// GPU Resources for the asset pointed to by the component
	// It is NOT safe to dereference this pointer on the game thread
	TSharedPtr<FGaussianSplatResourceWrapper> GaussianSplatResource_RenderThread;

	TWeakPtr<FGSViewExtension> ViewExtension;
};
