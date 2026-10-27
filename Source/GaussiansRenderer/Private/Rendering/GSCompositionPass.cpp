#include "GSViewExtension.h"

#include "RenderGraphBuilder.h"
#include "ScenePrivate.h"
#include "ScreenPass.h"
#include "SceneRendering.h"
#include "ShaderCompiler.h"
#include "Rendering/GSplatGBuffer.h"


static TAutoConsoleVariable<float> CVarGaussiansAlphaCutoff(
	TEXT("r.Gaussians.AlphaCutoff"),
	0.3f,
	TEXT("Threshold for compositing splats into the GBuffer. In range [0,1]."),
	ECVF_RenderThreadSafe
);

// Mirrored from Substrate.h - not marked for DLL export there so must copy over
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FGaussianSplattingSubstrateBasePassUniformParameters, )
	SHADER_PARAMETER_STRUCT_INCLUDE(FSubstrateCommonParameters, Common)
	SHADER_PARAMETER(int32, SliceStoringDebugSubstrateTreeDataWithoutMRT)
	SHADER_PARAMETER(int32, FirstSliceStoringSubstrateSSSDataWithoutMRT)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2DArray<uint>, MaterialTextureArrayUAVWithoutRTs)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float3>, OpaqueRoughRefractionTextureUAV)
END_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FGaussianSplattingSubstrateBasePassUniformParameters, "GaussianSplattingSubstrate");

namespace SubstrateHelpers
{

void SetSubstrateCommonParameters(FSubstrateCommonParameters& Parameters)
{
	Parameters.bRoughDiffuse = 0u;
	Parameters.MaxBytesPerPixel = 0u;
	Parameters.MaxClosurePerPixel = 0u;
	Parameters.PeelLayersAboveDepth = 0u;
	Parameters.bRoughnessTracking = 0u;
	Parameters.bStochasticLighting = 0u;
};

void SetSubstrateCommonParameters(const FSubstrateSceneData& SceneData, FSubstrateCommonParameters& Parameters)
{
	Parameters.bRoughDiffuse = SceneData.bRoughDiffuse ? 1u : 0u;
	Parameters.MaxBytesPerPixel = SceneData.EffectiveMaxBytesPerPixel;
	Parameters.MaxClosurePerPixel = SceneData.EffectiveMaxClosurePerPixel;
	Parameters.PeelLayersAboveDepth = SceneData.PeelLayersAboveDepth;
	Parameters.bRoughnessTracking = SceneData.bRoughnessTracking ? 1u : 0u;
	Parameters.bStochasticLighting = SceneData.bStochasticLighting ? 1u : 0u;
};

TRDGUniformBufferRef<FGaussianSplattingSubstrateBasePassUniformParameters> BuildSubstrateParameters(FRDGBuilder& GraphBuilder, const FViewInfo& ViewInfo)
{
	FGaussianSplattingSubstrateBasePassUniformParameters& SubstrateParameters = *GraphBuilder.AllocParameters<FGaussianSplattingSubstrateBasePassUniformParameters>();

	bool bCreateDummyResources = false;

	const FSubstrateSceneData* SubstrateSceneData = ViewInfo.SubstrateViewData.SceneData;
	if (Substrate::IsSubstrateEnabled() && SubstrateSceneData)
	{
		SetSubstrateCommonParameters(*SubstrateSceneData, SubstrateParameters.Common);
		if (SubstrateSceneData->MaterialTextureArrayUAVWithoutRTs)
		{
			SubstrateParameters.SliceStoringDebugSubstrateTreeDataWithoutMRT = SubstrateSceneData->SliceStoringDebugSubstrateTreeDataWithoutMRT;
			SubstrateParameters.FirstSliceStoringSubstrateSSSDataWithoutMRT = SubstrateSceneData->FirstSliceStoringSubstrateSSSDataWithoutMRT;
			SubstrateParameters.MaterialTextureArrayUAVWithoutRTs = SubstrateSceneData->MaterialTextureArrayUAVWithoutRTs;
			SubstrateParameters.OpaqueRoughRefractionTextureUAV = SubstrateSceneData->OpaqueRoughRefractionTextureUAV;
		}
		else
		{
			bCreateDummyResources = true;
		}
	}
	else
	{
		SetSubstrateCommonParameters(SubstrateParameters.Common);
		bCreateDummyResources = true;
	}

	if (bCreateDummyResources)
	{
		FRDGTextureRef DummyWritableRefracTexture = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(FIntPoint(1, 1), PF_R8, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("Substrate.DummyWritableTexture"));
		FRDGTextureUAVRef DummyWritableRefracTextureUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DummyWritableRefracTexture));

		FRDGTextureRef DummyWritableTextureArray = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2DArray(FIntPoint(1, 1), PF_R32_UINT, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV, 1), TEXT("Substrate.DummyWritableTexture"));
		FRDGTextureUAVRef DummyWritableTextureArrayUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DummyWritableTextureArray));

		SubstrateParameters.SliceStoringDebugSubstrateTreeDataWithoutMRT = -1;
		SubstrateParameters.FirstSliceStoringSubstrateSSSDataWithoutMRT = -1;
		SubstrateParameters.MaterialTextureArrayUAVWithoutRTs = DummyWritableTextureArrayUAV;
		SubstrateParameters.OpaqueRoughRefractionTextureUAV = DummyWritableRefracTextureUAV;
	}

	return GraphBuilder.CreateUniformBuffer<FGaussianSplattingSubstrateBasePassUniformParameters>(&SubstrateParameters);
}

}


class FGSCompositionPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSCompositionPS);
	SHADER_USE_PARAMETER_STRUCT(FGSCompositionPS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

		// Used when Substrate is enabled
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FGaussianSplattingSubstrateBasePassUniformParameters, SubstrateStruct)

		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, InputViewPort)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, OutputViewPort)

		// Splat buffer
		SHADER_PARAMETER_STRUCT_INCLUDE(FGSplatGBufferParameters, GSplatGBuffer)

		SHADER_PARAMETER(float, AlphaThreshold)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, RWVelocityTexture)

		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment
	)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		// We need to set these so that the shader generation utils will set up the GBuffer
		OutEnvironment.SetDefine(TEXT("IS_BASE_PASS"), true);
		OutEnvironment.SetDefine(TEXT("MATERIALBLENDING_SOLID"), true);
		OutEnvironment.SetDefine(TEXT("MATERIAL_SHADINGMODEL_DEFAULT_LIT"), true);

		// Substrate with Adaptive GBuffer requires extra handling.
		bool bSubstrateMode = Substrate::IsSubstrateEnabled() && !Substrate::IsSubstrateBlendableGBufferEnabled(Parameters.Platform);
		OutEnvironment.SetDefine(TEXT("MATERIAL_IS_SUBSTRATE"), bSubstrateMode);

		if (bSubstrateMode)
		{
			OutEnvironment.SetDefine(TEXT("SUBSTRATE_INLINE_SHADING"), 1);
			OutEnvironment.SetDefine(TEXT("SUBSTRATE_CLAMPED_CLOSURE_COUNT"), 1);

			FGBufferParams GBufferParams = FShaderCompileUtilities::FetchGBufferParamsRuntime(Parameters.Platform, GBL_Default);
			const FGBufferInfo BufferInfo = FetchFullGBufferInfo(GBufferParams);

			// Add N uint for Substrate fast path
			for (int i = 0; i < SUBSTRATE_BASE_PASS_MRT_OUTPUT_COUNT; ++i)
			{
				OutEnvironment.SetRenderTargetOutputFormat(BufferInfo.NumTargets + i, PF_R32_UINT);
			}

			// Add another MRT for Substrate top layer information
			OutEnvironment.SetRenderTargetOutputFormat(BufferInfo.NumTargets + SUBSTRATE_BASE_PASS_MRT_OUTPUT_COUNT, Substrate::GetNormalQuality() > 0 ? PF_R32G32_UINT : PF_R32_UINT);
		}
	}
};

IMPLEMENT_GLOBAL_SHADER(FGSCompositionPS, "/Plugin/StarLightGS/GSplatComposition.usf", "GSplatCompositionPS", SF_Pixel)


void FGSViewExtension::GSCompositionPass(
	FRDGBuilder& GraphBuilder, 
	const FSceneView& InView,
	const FGSplatGBuffer& GSplatGBuffer,
	const FRenderTargetBindingSlots& RenderTargets
)
{
	RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: GSplatGBuffer Composition");

	check(IsInRenderingThread());
	check(InView.bIsViewInfo);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);

	FGSCompositionPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGSCompositionPS::FParameters>();
	PassParameters->View = ViewInfo.ViewUniformBuffer;
	if (Substrate::IsSubstrateEnabled())
	{
		PassParameters->SubstrateStruct = SubstrateHelpers::BuildSubstrateParameters(GraphBuilder, ViewInfo);
	}

	// Bind splat buffer
	PassParameters->GSplatGBuffer = GSplatGBuffer.GetShaderParameters(GraphBuilder);

	PassParameters->AlphaThreshold = FMath::Clamp(CVarGaussiansAlphaCutoff.GetValueOnRenderThread(), 0.0f, 1.0f);

	// Get GBuffer
	const FSceneTextures& SceneTexturesData = ViewInfo.GetSceneTextures();

	FRDGTextureRef VelocityTexture = GetIfProduced(SceneTexturesData.Velocity);
	check(VelocityTexture);
	FRDGTextureRef DepthTexture = GetIfProduced(SceneTexturesData.Depth.Target);
	check(DepthTexture);

	PassParameters->RWVelocityTexture = GraphBuilder.CreateUAV(SceneTexturesData.Velocity);

	PassParameters->RenderTargets = RenderTargets;
	PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(
		DepthTexture, ERenderTargetLoadAction::ELoad,
		ERenderTargetLoadAction::ENoAction, FExclusiveDepthStencil::DepthWrite_StencilNop
	);

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(ViewInfo.FeatureLevel);
	TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);

	TShaderMapRef<FGSCompositionPS> PixelShader(ShaderMap);

	FScreenPassTextureViewport ViewPort(ViewInfo.ViewRect.Size());
	PassParameters->InputViewPort = GetScreenPassTextureViewportParameters(ViewPort);
	PassParameters->OutputViewPort = GetScreenPassTextureViewportParameters(ViewPort);

	using FCompositionDepthStencilState = TStaticDepthStencilState<true, CF_DepthNearOrEqual>;

	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("GSplatComposition"),
		ViewInfo,
		ViewPort,
		ViewPort,
		VertexShader,
		PixelShader,
		FCompositionDepthStencilState::GetRHI(),
		PassParameters
	);
}
