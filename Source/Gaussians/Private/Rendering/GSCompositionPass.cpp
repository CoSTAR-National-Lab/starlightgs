#include "GSViewExtension.h"

#include "RenderGraphBuilder.h"
#include "ScreenPass.h"
#include "SceneRendering.h"
#include "Rendering/GSplatGBuffer.h"


class FGSCompositionPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSCompositionPS);
	SHADER_USE_PARAMETER_STRUCT(FGSCompositionPS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, InputViewPort)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, OutputViewPort)

		// Splat buffer
		SHADER_PARAMETER_STRUCT_INCLUDE(FGSplatGBufferParameters, GSplatGBuffer)

		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment
	)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		// We need to set these so that the shader generation utils will set up the GBuffer
		// TODO: This does not work with Substrate
		OutEnvironment.SetDefine(TEXT("IS_BASE_PASS"), true);
		OutEnvironment.SetDefine(TEXT("MATERIALBLENDING_SOLID"), true);
		OutEnvironment.SetDefine(TEXT("MATERIAL_SHADINGMODEL_DEFAULT_LIT"), true);
	}
};

IMPLEMENT_GLOBAL_SHADER(FGSCompositionPS, "/Plugin/Gaussians/GSplatComposition.usf", "GSplatCompositionPS", SF_Pixel)


void FGSViewExtension::GSCompositionPass(
	FRDGBuilder& GraphBuilder, 
	const FSceneView& InView,
	const FGSplatGBuffer& GSplatGBuffer
)
{
	check(IsInRenderingThread());
	check(InView.bIsViewInfo);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);

	FGSCompositionPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGSCompositionPS::FParameters>();
	PassParameters->View = ViewInfo.ViewUniformBuffer;

	// Bind splat buffer
	PassParameters->GSplatGBuffer = GSplatGBuffer.GetShaderParameters(GraphBuilder);

	// Get GBuffer
	const FSceneTextures& SceneTexturesData = ViewInfo.GetSceneTextures();
	TStaticArray<FTextureRenderTargetBinding, MaxSimultaneousRenderTargets> RenderTargetTextures;
	uint32 RenderTargetTextureCount = SceneTexturesData.GetGBufferRenderTargets(RenderTargetTextures);
	TArrayView<FTextureRenderTargetBinding> RenderTargetTexturesView(RenderTargetTextures.GetData(), RenderTargetTextureCount);

	FRDGTextureRef DepthTexture = GetIfProduced(SceneTexturesData.Depth.Target);
	check(DepthTexture);

	PassParameters->RenderTargets = GetRenderTargetBindings(ERenderTargetLoadAction::ELoad, RenderTargetTexturesView);
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
