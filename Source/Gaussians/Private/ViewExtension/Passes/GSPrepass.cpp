
#include "GaussianSplatResource.h"
#include "GaussianSplatSceneProxy.h"
#include "GSViewExtension.h"
#include "HLSLTypeAliases.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RendererInterface.h"
#include "RendererClasses.h"
#include "ScreenPass.h"
#include "DSP/DelayStereo.h"
#include "Runtime/Renderer/Internal/PathTracingResources.h"
#include "Runtime/Renderer/Internal/SceneTextures.h"
#include "Runtime/Renderer/Internal/TranslucentPassResource.h"
#include "Rendering/GPURadixSort.h"

//class FGSCompute : public FGlobalShader
//{
//	DECLARE_GLOBAL_SHADER(FGSCompute)
//	SHADER_USE_PARAMETER_STRUCT(FGSCompute, FGlobalShader)
//
//	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
//		//SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformBuffer, SceneParams)
//
//		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, Output)
//		SHADER_PARAMETER(FVector4f, Colour)
//		//	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FPrismBarycentric>, Barycentrics)
//		//	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, FaceIndex)
//		//	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, Scales)
//		//	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, Orientations)
//		//	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SHAndOpacities)
//	END_SHADER_PARAMETER_STRUCT()
//};
//
//IMPLEMENT_GLOBAL_SHADER(FGSCompute,"/Plugin/Gaussians/GSCompute.usf", "MainCS", SF_Compute);

struct FInstanceData
{
	uint32 TileID;
	uint32 Depth;
	uint32 GaussianID;
};

//Cull and Bin Shader
class FGSPrepass : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSPrepass)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		//SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformBuffer, SceneParams)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_STRUCT_REF(FGaussianSplatShaderResources, GSResource)
		SHADER_PARAMETER_STRUCT_REF(FPrimitiveUniformShaderParameters, Primitive)

		//in 
		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, InScales)
		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float3>, InPositions)
		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, InOrientations)

		//SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, InColorOpacity)

		//out
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint32>, Counter)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float3>, OutColor)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, OutRadii)
		//SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, OutConicOpacity)
		//RDG_BUFFER_ACCESS()
		
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float2>, OutSSPositions)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, OutSSScale)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutConic)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, OutColorOpacity)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutDepth)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutGaussianIDX)
	END_SHADER_PARAMETER_STRUCT()

	SHADER_USE_PARAMETER_STRUCT(FGSPrepass, FGlobalShader);
};

IMPLEMENT_GLOBAL_SHADER(FGSPrepass, "/Plugin/Gaussians/GSCompute.usf", "GSPrepass", SF_Compute);


void FGSViewExtension::GSPrePass(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	if (Proxies.Num())
	{
		for (auto Proxy : Proxies)
		{
			FGaussianSplatResource* Resource = Proxy->GetResource_RenderThread();

			auto count  = Resource->GetSplatCount();

			//out
			auto CounterBufferDesc = FRDGBufferDesc::CreateIndirectDesc<FRHIDrawIndirectParameters>();

			auto OutNDCPositionsDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector2f), count);
			auto OutNDCScalesDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), count);
			auto OutConicsDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), count);
			auto OutColorOpacityDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), count);

			auto OutGaussianIDXDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), count);
			auto OutDepthDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), count);

			TransientBuffers.Add(TEXT("CounterBuffer"), GraphBuilder.CreateBuffer(CounterBufferDesc, TEXT("Counter")));
			TransientBuffers.Add(TEXT("OutNDCPositionsBuffer"), GraphBuilder.CreateBuffer(OutNDCPositionsDesc, TEXT("OutNDCPositions")));
			TransientBuffers.Add(TEXT("OutNDCScalesBuffer"), GraphBuilder.CreateBuffer(OutNDCScalesDesc, TEXT("OutNDCScales")));
			TransientBuffers.Add(TEXT("OutConic"), GraphBuilder.CreateBuffer(OutConicsDesc, TEXT("OutConics")));
			TransientBuffers.Add(TEXT("OutColorOpacity"), GraphBuilder.CreateBuffer(OutColorOpacityDesc, TEXT("OutColorOpacity")));
			TransientBuffers.Add(TEXT("OutGaussianIDX"), GraphBuilder.CreateBuffer(OutGaussianIDXDesc, TEXT("OutGaussianIDX")));
			TransientBuffers.Add(TEXT("OutDepth"), GraphBuilder.CreateBuffer(OutDepthDesc, TEXT("OutDepth")));	

			TUniformBufferRef<FGaussianSplatShaderResources> ResourceUniformBuffer = TUniformBufferRef<FGaussianSplatShaderResources>{ Resource->GetUniformBuffer() };
			TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer = TUniformBufferRef<FPrimitiveUniformShaderParameters>{ Proxy->GetUniformBuffer() };

			auto Parameters = GraphBuilder.AllocParameters<FGSPrepass::FParameters>();

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

			//clear Indirect ARGS
			AddClearUAVPass(GraphBuilder, Parameters->Counter, 0);
			//non count related value, like vertex count will be set by dispatch thread 0

			//do compute
			const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetShaderPlatform());
			TShaderMapRef<FGSPrepass> ComputeShader(ShaderMap);

			int XSize = count / 256; //256 is the thread group size in the shader, so dispatch size is number of thread groups needed to process all splats
			FIntVector dispatchsize = { XSize, 1, 1 };

			FComputeShaderUtils::AddPass<FGSPrepass>(

				GraphBuilder,
				RDG_EVENT_NAME("GSPrepass"),
				ComputeShader,
				Parameters,
				dispatchsize
			);

			auto pingpongvalue = GraphBuilder.CreateBuffer(OutGaussianIDXDesc, TEXT("PingpongIDX"));
			auto pingpongkey = GraphBuilder.CreateBuffer(OutDepthDesc, TEXT("PingpongValue"));

			TArray<FRDGBufferRef> InValues = { TransientBuffers[TEXT("OutGaussianIDX")], pingpongvalue };
			TArray<FRDGBufferRef> InKeys = {   TransientBuffers[TEXT("OutDepth")], pingpongkey };


			auto index = GPURadixSort::SortBufferIndirect(GraphBuilder,InKeys,InValues, 0, TransientBuffers[TEXT("CounterBuffer")],1, UINT32_MAX, GMaxRHIFeatureLevel);

			TransientBuffers[TEXT("OutGaussianIDX")] = InValues[index];
			TransientBuffers[TEXT("OutDepth")] = InKeys[index];

			// run one instance for the start once
			break;
		}
	}
}
