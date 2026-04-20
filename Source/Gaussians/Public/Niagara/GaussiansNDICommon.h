#pragma once

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "UnifiedBuffer.h"

struct RDGBufferStruct
{
	TRefCountPtr<FRDGPooledBuffer>	PooledBuffer;
	FRDGBufferRef					RDGTransientBuffer = nullptr;
	FRDGBufferSRVRef				RDGTransientBufferSRV = nullptr;

	void Release()
	{
		PooledBuffer.SafeRelease();
	}
};
