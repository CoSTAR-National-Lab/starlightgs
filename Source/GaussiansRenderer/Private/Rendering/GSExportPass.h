#pragma once

#include "RenderGraphFwd.h"
#include "RHIResources.h"

class IGaussianSplatInterface;


// Buffers holding data being read back from GPU to CPU
struct FGaussianSplatReadbackData
{
	// Actual splat count (post-culling) may only be known on the GPU, so it must be readback also
	FStagingBufferRHIRef SplatCount;

	// Per-splat properties
	FStagingBufferRHIRef Positions;
	FStagingBufferRHIRef Scales;
	FStagingBufferRHIRef Orientations;
	FStagingBufferRHIRef SHAndOpacities;

	// Valid if relighting is enabled on the readback splat, otherwise nullptr
	FStagingBufferRHIRef Normals;
	FStagingBufferRHIRef MetallicRoughness;

	// The fence to wait on to ensure GPU work populating readback buffers has completed
	FGPUFenceRHIRef Fence;
};

namespace Gaussians
{
	void ExportSplatData(
		FRDGBuilder& GraphBuilder,
		const IGaussianSplatInterface* Proxy,
		FGaussianSplatReadbackData& OutReadbackData
	);
}
