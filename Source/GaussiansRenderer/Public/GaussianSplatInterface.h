#pragma once


#include "CoreMinimal.h"

class FGaussianSplatResource;


// Describes functionality of a renderable gaussian splat across module boundaries
// Implemented by GaussianSplatSceneProxy
class GAUSSIANSRENDERER_API IGaussianSplatInterface
{
public:
	virtual ~IGaussianSplatInterface() = default;

	virtual FGaussianSplatResource* GetResource_RenderThread() const = 0;
	virtual FRHIUniformBuffer* GetProxyUniformBuffer() const = 0;

	virtual const FBoxSphereBounds& GetSplatBounds() const = 0;
	virtual bool IsSplatShown(const FSceneView* InView) const = 0;

	virtual FRHIShaderResourceView* GetIndexBufferSRV() const = 0;
	virtual FRHIUnorderedAccessView* GetIndexBufferUAV() const = 0;

	virtual FRHIBuffer* GetIndexCountBuffer() const = 0;
	virtual FRHIShaderResourceView* GetIndexCountBufferSRV() const = 0;
	virtual FRHIUnorderedAccessView* GetIndexCountBufferUAV() const = 0;

	virtual bool RequiresNewCullingPass() const = 0;
	virtual void MarkIndexBufferUpdated() = 0;

	virtual bool SupportsRelighting() const = 0;

	virtual float GetScaleMultiplier() const = 0;
	virtual int32 GetClippingVolumeMask() const = 0;
};
