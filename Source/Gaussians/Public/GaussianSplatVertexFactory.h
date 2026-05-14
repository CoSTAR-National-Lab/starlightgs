

#pragma once

#include "CoreMinimal.h"

/**
 * Gaussian splat factory that fetches splat data form asset uniform and passes it to the mesh pipeline manually
 * used for rendering masked shadow for the gaussian splatting
 */

class FGaussianSplatVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE_API(FGaussianSplatVertexFactory, GAUSSIANS_API)
public:

	FGaussianSplatVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	virtual bool RendersPrimitivesAsCameraFacingSprites() const override
	{
		return true;
	};

	
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	
	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	void SetGSplatDataUniformBuffer(FUniformBufferRHIRef InBuffer) { GSplatDataUniform = InBuffer; }
	FRHIUniformBuffer* GetGSplatDataUniformBuffer() const { return GSplatDataUniform; }

private:

	FUniformBufferRHIRef	GSplatDataUniform;

};
