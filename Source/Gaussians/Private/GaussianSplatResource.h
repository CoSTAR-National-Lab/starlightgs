#pragma once

#include "RenderResource.h"
#include "GaussianData.h"


BEGIN_UNIFORM_BUFFER_STRUCT(FGaussianSplatShaderResources,)
	SHADER_PARAMETER(int32, SplatCount)

	// Required buffers
	SHADER_PARAMETER_SRV(StructuredBuffer<float3>, Positions)
	SHADER_PARAMETER_SRV(StructuredBuffer<float3>, Scales)
	SHADER_PARAMETER_SRV(StructuredBuffer<float4>, Orientations)
	SHADER_PARAMETER_SRV(StructuredBuffer<float4>, SHAndOpacities)

	// Optional buffers
	SHADER_PARAMETER_SRV(StructuredBuffer<float3>, Normals)
	SHADER_PARAMETER_SRV(StructuredBuffer<float>, HighOrderSH)
	SHADER_PARAMETER_SRV(StructuredBuffer<float2>, MetallicRoughness)
END_UNIFORM_BUFFER_STRUCT()


struct FGaussianSplatData;


/**
 * Container of all GPU resources required to render a gaussian splat asset.
 * This takes a Structure of Arrays approach, where each property has its own array.
 *
 * All GPU resources are packaged into a uniform buffer.
 * This uniform buffer can be consumed by rendering pipelines.
 * 
 * Not all arrays must exist - some properties are optional and will only be created
 * when the imported Gaussian splat asset supplied data.
 *
 * While any missing properties will still have a valid SRV in the uniform buffer,
 * they should not be accessed in shaders.
 */
class FGaussianSplatResource : public FRenderResource
{
public:
	// Makes a copy of the splat data that will be read on the render thread when RHI is initialized
	// TODO: Could potentially release CPU splat data once copied to GPU
	FGaussianSplatResource(FGaussianSplatData InSplatData);
	virtual ~FGaussianSplatResource();

	//~ Begin FRenderResource Interface
	virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
	virtual void ReleaseRHI() override;

	virtual FString GetFriendlyName() const override { return TEXT("GaussianSplatResource"); }
	//~ End FRenderResource Interface


	// Call this when splats have changed on CPU. It will copy splats over to the GPU.
	// Call this on the rendering thread only (with a copy of the splat data)
	void UpdateSplats(FRHICommandListBase& RHICmdList, FGaussianSplatData InSplatData);

	// The uniform buffer collects all shader parameters related to this splat model
	// Can be nullptr if resources have not been created or this resource has no data
	// Check it is not nullptr before use!
	FRHIUniformBuffer* GetUniformBuffer() { return UniformBuffer; }

	// Gets splat count from CPU copy of splat data
	uint32 GetSplatCount() const { return SplatData.SplatCount; }
	// Checks from CPU copy of splat data if this resource supports relighting
	bool SupportsRelighting() const { return SplatData.SupportsRelighting(); }

private:
	// CPU copy of data that this resource will copy to GPU
	FGaussianSplatData SplatData;

	TUniformBufferRef<FGaussianSplatShaderResources> UniformBuffer{ };

	struct FBufferResources
	{
		FBufferRHIRef Buffer;
		FShaderResourceViewRHIRef SRV;

		void SafeRelease()
		{
			SRV.SafeRelease();
			Buffer.SafeRelease();
		}
	};

	// Required buffers
	FBufferResources PositionsBuffer;
	FBufferResources ScalesBuffer;
	FBufferResources OrientationsBuffer;
	FBufferResources SHAndOpacitiesBuffer;

	// Optional buffers
	FBufferResources NormalsBuffer;
	FBufferResources HighOrderSHBuffer;
	FBufferResources MetallicRoughnessBuffer;
};
