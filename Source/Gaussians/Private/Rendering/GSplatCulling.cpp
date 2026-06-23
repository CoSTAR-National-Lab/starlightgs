#include "GSViewExtension.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RendererInterface.h"
#include "RendererClasses.h"
#include "ScreenPass.h"
#include "ScenePrivate.h"

#include "GaussianSplatResource.h"
#include "GaussianSplatSceneProxy.h"

#include "GaussiansDefinitions.h"


class FGSplatCullingCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSplatCullingCS)
	SHADER_USE_PARAMETER_STRUCT(FGSplatCullingCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_STRUCT_REF(FGaussianSplatShaderResources, GSResource)
		SHADER_PARAMETER_STRUCT_REF(FPrimitiveUniformShaderParameters, Primitive)

		SHADER_PARAMETER(float, SplatScaleMultiplier)

		SHADER_PARAMETER(int32, NumClippingVolumes)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FGaussians_ClippingVolume>, ClippingVolumes)
		SHADER_PARAMETER(uint32, ClippingVolumeMask)

		// Out
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, RWSplatIndexBufferAllocator)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, RWSplatIndices)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_1D"), GetThreadGroupSize1D());
	}

	static uint32 GetThreadGroupSize1D() { return 128; }
};

IMPLEMENT_GLOBAL_SHADER(FGSplatCullingCS, "/Plugin/Gaussians/GSplatCulling.usf", "GSplatCullingCS", SF_Compute);


int32 ConvertClippingMode(EGaussianSplatClippingVolumeMode Mode)
{
	switch (Mode) {
	case EGaussianSplatClippingVolumeMode::ClipAnyInside:  return GAUSSIANS_CLIPPING_MODE_CLIP_INSIDE_ANY;
	case EGaussianSplatClippingVolumeMode::ClipAllInside:  return GAUSSIANS_CLIPPING_MODE_CLIP_INSIDE_ALL;
	case EGaussianSplatClippingVolumeMode::ClipAnyOutside: return GAUSSIANS_CLIPPING_MODE_CLIP_OUTSIDE_ANY;
	case EGaussianSplatClippingVolumeMode::ClipAllOutside: return GAUSSIANS_CLIPPING_MODE_CLIP_OUTSIDE_ALL;
	default:
		checkNoEntry(); break;
	}
	return -1;
}


void FGSViewExtension::GSCullingPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& InView,
	const TSet<FGaussianSplatSceneProxy*>& Proxies,
	const TArray<FGSClippingVolumeParameters>& CullingVolumes,
	bool bForceEnable
)
{
	check(InView.bIsViewInfo);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);

	RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Culling");

	const uint32 NumCullingVolumes = CullingVolumes.Num();
	FRDGBufferRef CullingVolumesBuffer = NumCullingVolumes == 0
		? GSystemTextures.GetDefaultStructuredBuffer(GraphBuilder, sizeof(FGaussians_ClippingVolume))
		: GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(FGaussians_ClippingVolume), NumCullingVolumes),
			TEXT("Gaussians.ClippingVolumes")
		);

	if (NumCullingVolumes > 0)
	{
		FGaussians_ClippingVolume* ClippingVolumeShaderData = GraphBuilder.AllocPODArray<FGaussians_ClippingVolume>(NumCullingVolumes);
		//const FVector& PreViewTranslation = ViewInfo.ViewMatrices.GetPreViewTranslation();
		const FVector& PreViewTranslation = FVector::ZeroVector;

		auto WritePtr = ClippingVolumeShaderData;
		for (const auto& Volume : CullingVolumes)
		{
			WritePtr->Min = static_cast<FVector3f>(Volume.Bounds.Min + PreViewTranslation);
			WritePtr->Max = static_cast<FVector3f>(Volume.Bounds.Max + PreViewTranslation);
			WritePtr->Mode = ConvertClippingMode(Volume.Mode);
			WritePtr->Mask = Volume.Mask;

			WritePtr++;
		}

		// Safe to no copy since initial data was allocated via graph builder
		GraphBuilder.QueueBufferUpload(CullingVolumesBuffer, ClippingVolumeShaderData, NumCullingVolumes * sizeof(FGaussians_ClippingVolume), ERDGInitialDataFlags::NoCopy);
	}

	for (FGaussianSplatSceneProxy* Proxy : Proxies)
	{
		// Check if culling required
		if (!Proxy->RequiresNewCullingPass() && !bForceEnable)
			continue;

		FRHIUnorderedAccessView* IndexBuffer = Proxy->GetIndexBufferUAV();
		FRHIUnorderedAccessView* CountBuffer = Proxy->GetIndexCountBufferUAV();

		// Clear counter
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("GSplatClearIndexCounter"),
			ERDGPassFlags::NeverCull,
			[CountBuffer](FRHICommandListImmediate& RHICmdList)
			{
				FUintVector4 Values{ 0, 0, 0, 0 };
				RHICmdList.ClearUAVUint(CountBuffer, Values);
			}
		);

		{
			TUniformBufferRef<FGaussianSplatShaderResources> ResourceUniformBuffer = TUniformBufferRef<FGaussianSplatShaderResources>{ Proxy->GetResource_RenderThread()->GetUniformBuffer() };
			TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer = TUniformBufferRef<FPrimitiveUniformShaderParameters>{ Proxy->GetUniformBuffer() };

			FGSplatCullingCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGSplatCullingCS::FParameters>();
			PassParameters->View = ViewInfo.ViewUniformBuffer;
			PassParameters->GSResource = ResourceUniformBuffer;
			PassParameters->Primitive = PrimitiveUniformBuffer;

			PassParameters->SplatScaleMultiplier = Proxy->GetScaleMultiplier();

			PassParameters->NumClippingVolumes = NumCullingVolumes;
			PassParameters->ClippingVolumes = GraphBuilder.CreateSRV(CullingVolumesBuffer);
			PassParameters->ClippingVolumeMask = Proxy->GetClippingVolumeMask();

			PassParameters->RWSplatIndexBufferAllocator = CountBuffer;
			PassParameters->RWSplatIndices = IndexBuffer;

			FIntVector DispatchSize = FComputeShaderUtils::GetGroupCountWrapped(Proxy->GetResource_RenderThread()->GetSplatCount(), FGSplatCullingCS::GetThreadGroupSize1D());

			const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InView.GetShaderPlatform());
			TShaderMapRef<FGSplatCullingCS> ComputeShader(ShaderMap);

			FComputeShaderUtils::AddPass<FGSplatCullingCS>(
				GraphBuilder,
				RDG_EVENT_NAME("GSplatCulling"),
				ComputeShader,
				PassParameters,
				DispatchSize
			);
		}

		Proxy->MarkIndexBufferUpdated();
	}
}
