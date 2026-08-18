#pragma once


class FGSGPUStats
{
public:
	enum EStatID
	{
		Stat_LitSplatsRendered = 0,
		Stat_UnlitSplatsRendered,

		Stat_Count
	};

	// Call in begin render view family on render thread
	void ProcessBeginRender(FRDGBuilder& GraphBuilder);
	// Call in post render view family on render thread
	void ProcessEndRender(FRDGBuilder& GraphBuilder);

	// Call to update stats for a frame with the entry in SourceData at offset OffsetInBytes
	void SetStatFromGPU(FRDGBuilder& GraphBuilder, EStatID Stat, FRDGBufferRef SourceData, uint32 OffsetInBytes) const;

private:
	// Must queue readback and wait until work completed on GPU before reading
	struct FGPUStat
	{
		FStagingBufferRHIRef StagingBuffer;	// Buffer containing data to be read on CPU
		FGPUFenceRHIRef Fence;				// Fence to indicate when data is ready to be read
	};
	TQueue<FGPUStat> StatsReadback;

	// Holds stats data throughout the frame. At the end of the frame it is copied into a readback buffer
	FRDGBufferRef StatsTransientBuffer = nullptr;
};
