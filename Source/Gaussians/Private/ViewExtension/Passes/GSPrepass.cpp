
#include "GaussianSplatResource.h"
#include "GaussianSplatSceneProxy.h"
#include "GSViewExtension.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RendererInterface.h"
#include "RendererClasses.h"
#include "ScreenPass.h"

#include "Rendering/GPURadixSort.h"


static TAutoConsoleVariable<int32> CVarGaussiansMaxSplats(
	TEXT("r.Gaussians.MaxSplats"),
	5'000'000,
	TEXT("The maximum number of splats that can be rendered in a single frame."),
	ECVF_RenderThreadSafe
);


struct FInstanceData
{
	uint32 TileID;
	uint32 Depth;
	uint32 GaussianID;
};

//Cull and Bin Shader
class FGSPrepassCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSPrepassCS)
	SHADER_USE_PARAMETER_STRUCT(FGSPrepassCS, FGlobalShader);

	class FEnableRelighting : SHADER_PERMUTATION_BOOL("ENABLE_RELIGHTING");
	using FPermutationDomain = TShaderPermutationDomain<FEnableRelighting>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_STRUCT_REF(FGaussianSplatShaderResources, GSResource)
		SHADER_PARAMETER_STRUCT_REF(FPrimitiveUniformShaderParameters, Primitive)

		// Out
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint32>, Counter)
		
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float2>, OutSSPositions)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, OutSSScale)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutConic)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, OutColorOpacity)

		// Only used when relighting is enabled
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutNormal)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float2>, OutMR)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutDepth)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutGaussianIDX)

		SHADER_PARAMETER(uint32, BufferCapacity)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_1D"), GetThreadGroupSize1D());
	}

	static uint32 GetThreadGroupSize1D() { return 256; }
};

IMPLEMENT_GLOBAL_SHADER(FGSPrepassCS, "/Plugin/Gaussians/GSCompute.usf", "GSPrepass", SF_Compute);


void FGSViewExtension::GSPrePass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const TSet<FGaussianSplatSceneProxy*>& Proxies)
{
	// Don't call this when there are no proxies
	check(!Proxies.IsEmpty());
	bool bRelighting = (*Proxies.FindArbitraryElement())->SupportsRelighting();

	// Create transient buffers
	auto CounterBufferDesc = FRDGBufferDesc::CreateIndirectDesc<FRHIDrawIndirectParameters>();

	int32 Capacity = CVarGaussiansMaxSplats.GetValueOnRenderThread();
	auto OutNDCPositionsDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector2f), Capacity);
	auto OutNDCScalesDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), Capacity);
	auto OutConicsDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), Capacity);
	auto OutColorOpacityDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), Capacity);

	auto OutGaussianIDXDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Capacity);
	auto OutDepthDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Capacity);

	TransientBuffers.Add(TEXT("CounterBuffer"), GraphBuilder.CreateBuffer(CounterBufferDesc, TEXT("Counter")));
	TransientBuffers.Add(TEXT("OutNDCPositionsBuffer"), GraphBuilder.CreateBuffer(OutNDCPositionsDesc, TEXT("OutNDCPositions")));
	TransientBuffers.Add(TEXT("OutNDCScalesBuffer"), GraphBuilder.CreateBuffer(OutNDCScalesDesc, TEXT("OutNDCScales")));
	TransientBuffers.Add(TEXT("OutConic"), GraphBuilder.CreateBuffer(OutConicsDesc, TEXT("OutConics")));
	TransientBuffers.Add(TEXT("OutColorOpacity"), GraphBuilder.CreateBuffer(OutColorOpacityDesc, TEXT("OutColorOpacity")));

	TransientBuffers.Add(TEXT("OutGaussianIDX"), GraphBuilder.CreateBuffer(OutGaussianIDXDesc, TEXT("OutGaussianIDX")));
	TransientBuffers.Add(TEXT("OutDepth"), GraphBuilder.CreateBuffer(OutDepthDesc, TEXT("OutDepth")));

	if (bRelighting)
	{
		auto OutNormalDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), Capacity);
		auto OutMRDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector2f), Capacity);

		TransientBuffers.Add(TEXT("OutNormal"), GraphBuilder.CreateBuffer(OutNormalDesc, TEXT("OutNormal")));
		TransientBuffers.Add(TEXT("OutMR"), GraphBuilder.CreateBuffer(OutMRDesc, TEXT("OutMR")));
	}

	// Clear Indirect ARGS
	// Vertex count per instance will be set by dispatch thread 0
	AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(TransientBuffers[TEXT("CounterBuffer")], PF_R32_UINT), 0);

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetShaderPlatform());

	FGSPrepassCS::FPermutationDomain Permutation;
	Permutation.Set<FGSPrepassCS::FEnableRelighting>(bRelighting);
	TShaderMapRef<FGSPrepassCS> PrePassShader(ShaderMap, Permutation);

	for (auto Proxy : Proxies)
	{
		FGaussianSplatResource* Resource = Proxy->GetResource_RenderThread();

		auto SplatCount  = Resource->GetSplatCount();

		TUniformBufferRef<FGaussianSplatShaderResources> ResourceUniformBuffer = TUniformBufferRef<FGaussianSplatShaderResources>{ Resource->GetUniformBuffer() };
		TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer = TUniformBufferRef<FPrimitiveUniformShaderParameters>{ Proxy->GetUniformBuffer() };

		auto Parameters = GraphBuilder.AllocParameters<FGSPrepassCS::FParameters>();

		Parameters->View = InView.ViewUniformBuffer;
		Parameters->GSResource = ResourceUniformBuffer;
		Parameters->Primitive = PrimitiveUniformBuffer;

		Parameters->Counter = GraphBuilder.CreateUAV(TransientBuffers[TEXT("CounterBuffer")],				 PF_R32_UINT);

		Parameters->OutSSPositions = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutNDCPositionsBuffer")], PF_G32R32F);
		Parameters->OutSSScale = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutNDCScalesBuffer")],		 PF_A32B32G32R32F);
		Parameters->OutConic = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutConic")]);
		Parameters->OutColorOpacity = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutColorOpacity")],		 PF_A32B32G32R32F);
		Parameters->OutGaussianIDX = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutGaussianIDX")],		 PF_R32_UINT);
		Parameters->OutDepth = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutDepth")],					 PF_R32_UINT);

		if (bRelighting)
		{
			Parameters->OutNormal = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutNormal")]);
			Parameters->OutMR = GraphBuilder.CreateUAV(TransientBuffers[TEXT("OutMR")], PF_G32R32F);
		}

		Parameters->BufferCapacity = Capacity;

		FIntVector DispatchSize = FComputeShaderUtils::GetGroupCount(SplatCount, FGSPrepassCS::GetThreadGroupSize1D());

		FComputeShaderUtils::AddPass<FGSPrepassCS>(
			GraphBuilder,
			RDG_EVENT_NAME("GSPrepass"),
			PrePassShader,
			Parameters,
			DispatchSize
		);
	}

	// Sort all projected screens-space splats along view axis
	{
		RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Radix Sort");

		auto pingpongvalue = GraphBuilder.CreateBuffer(OutGaussianIDXDesc, TEXT("PingpongIDX"));
		auto pingpongkey = GraphBuilder.CreateBuffer(OutDepthDesc, TEXT("PingpongValue"));

		TArray<FRDGBufferRef> InValues = { TransientBuffers[TEXT("OutGaussianIDX")], pingpongvalue };
		TArray<FRDGBufferRef> InKeys = { TransientBuffers[TEXT("OutDepth")], pingpongkey };

		uint32 Index = GPURadixSort::SortBufferIndirect(
			GraphBuilder, 
			InKeys,
			InValues, 
			0,
			TransientBuffers[TEXT("CounterBuffer")],
			1,
			UINT32_MAX, 
			GMaxRHIFeatureLevel
		);

		TransientBuffers[TEXT("OutGaussianIDX")] = InValues[Index];
		TransientBuffers[TEXT("OutDepth")] = InKeys[Index];
	}
}
