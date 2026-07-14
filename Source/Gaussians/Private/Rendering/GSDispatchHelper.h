#pragma once

#include "RenderGraphFwd.h"


namespace Gaussians
{
	void AddSetupDispatchIndirectArgsPass(
		FRDGBuilder& GraphBuilder,
		FRHIShaderResourceView* CountSRV,
		FRDGBuffer* IndirectArgsBuffer,
		uint32 ThreadGroupSize1D
	);
}
