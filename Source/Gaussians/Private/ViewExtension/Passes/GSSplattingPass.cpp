#include "FXRenderingUtils.h"
#include "GSViewExtension.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "PixelShaderUtils.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessInputs.h"


class FSplattingPassVS :  public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSplattingPassVS)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float2>, VS_SSPositions)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, VS_SSScale)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float3>, VS_Conic)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, VS_ColorOpacity)

		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, VS_Depth)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, VS_GaussianIDX)

		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		
	END_SHADER_PARAMETER_STRUCT()

	SHADER_USE_PARAMETER_STRUCT(FSplattingPassVS, FGlobalShader);
};

IMPLEMENT_GLOBAL_SHADER(FSplattingPassVS, "/Plugin/Gaussians/GSCompute.usf", "GSSplatting_VS", SF_Vertex);

class FSplattingPassPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSplattingPassPS)
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		RDG_BUFFER_ACCESS(IndirectArgs, ERHIAccess::IndirectArgs)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	SHADER_USE_PARAMETER_STRUCT(FSplattingPassPS, FGlobalShader);
};

IMPLEMENT_GLOBAL_SHADER(FSplattingPassPS, "/Plugin/Gaussians/GSCompute.usf", "GSSplatting_PS", SF_Pixel);


BEGIN_SHADER_PARAMETER_STRUCT(FSplattingPassPipelineParameters, )
	SHADER_PARAMETER_STRUCT_INCLUDE(FSplattingPassVS::FParameters, VertexShader)
	SHADER_PARAMETER_STRUCT_INCLUDE(FSplattingPassPS::FParameters, PixelShader)
END_SHADER_PARAMETER_STRUCT()


void FGSViewExtension::GSSplattingPass(FRDGBuilder& GraphBuilder, const FSceneView& InView)
{
	// Don't call this when there are no proxies
	// TODO: Maybe relax this contract since it doesn't directly access proxy data? But still effectively no-ops if there are no proxies
	check(!Proxies.IsEmpty());

	auto PassParameters = GraphBuilder.AllocParameters<FSplattingPassPipelineParameters>();

	PassParameters->VertexShader.View = InView.ViewUniformBuffer;
	PassParameters->VertexShader.VS_SSPositions = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutNDCPositionsBuffer")], PF_G32R32F);
	PassParameters->VertexShader.VS_SSScale = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutNDCScalesBuffer")], PF_A32B32G32R32F);
	PassParameters->VertexShader.VS_Conic = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutConic")]);
	PassParameters->VertexShader.VS_ColorOpacity = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutColorOpacity")], PF_A32B32G32R32F);
	PassParameters->VertexShader.VS_GaussianIDX = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutGaussianIDX")], PF_R32_UINT);
	PassParameters->VertexShader.VS_Depth = GraphBuilder.CreateSRV(TransientBuffers[TEXT("OutDepth")], PF_R32_UINT);	

	PassParameters->PixelShader.IndirectArgs = TransientBuffers[TEXT("CounterBuffer")];
	PassParameters->PixelShader.RenderTargets[0] = FRenderTargetBinding(GSplatGBuffer.GetBaseColorTexture(), ERenderTargetLoadAction::ELoad, 0);

	// 5. Get Shader maps and shaders
	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetShaderPlatform());

	TShaderMapRef<FSplattingPassVS> VertexShader(ShaderMap);
	TShaderMapRef<FSplattingPassPS> PixelShader(ShaderMap);

	// 6. Add the pass
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("GSRasterizePass"),
		PassParameters,
		ERDGPassFlags::Raster,
		[PassParameters, VertexShader, PixelShader](FRHICommandList& RHICmdList)
		{
			// Initialize the Pipeline State
			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			// Set up blend/depth/rasterizer states
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA,BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

			// Set shaders
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI; //using SV_VertexID
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

			// Set shader parameters
			SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), PassParameters->VertexShader);
			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters->PixelShader);

			// Execute the Indirect Draw
			// The RHI needs the actual underlying FRHIBuffer for the indirect call
			FRHIBuffer* IndirectArgsRHIBuffer = PassParameters->PixelShader.IndirectArgs->GetIndirectRHICallBuffer();
			uint32 ArgumentOffset = 0; // Offset in bytes if your buffer contains multiple draw args

			RHICmdList.DrawPrimitiveIndirect(IndirectArgsRHIBuffer, ArgumentOffset);
		}
	);
}
