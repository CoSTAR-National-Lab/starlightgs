


#include "GaussianSplatVertexFactory.h"
#include "GaussianSplatResource.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

IMPLEMENT_VERTEX_FACTORY_TYPE(FGaussianSplatVertexFactory, "/Plugin/Gaussians/GSVertexFactory.ush",
	  EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsStaticLighting
	| EVertexFactoryFlags::SupportsManualVertexFetch
//	| EVertexFactoryFlags::SupportsPositionOnly
	//| EVertexFactoryFlags::SupportsRayTracing
	//| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
	//| EVertexFactoryFlags::SupportsPSOPrecaching
)

FGaussianSplatVertexFactory::FGaussianSplatVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : FVertexFactory(InFeatureLevel)
{
	//we dont need declartion as we are fetching data from buffers directly
	bNeedsDeclaration = false; // it will crash still if you do not create a declaration
};

class FGaussianSplatVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FGaussianSplatVertexFactoryShaderParameters, NonVirtual);
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		//ParameterMap.
	}

	void GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType VertexStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
		FVertexFactoryShaderParameters::GetElementShaderBindings(Scene, View, Shader, VertexStreamType, FeatureLevel, VertexFactory, BatchElement, ShaderBindings, VertexStreams);

		const FGaussianSplatVertexFactory* SplateVF = static_cast<const FGaussianSplatVertexFactory*>(VertexFactory);
		ShaderBindings.Add(Shader->GetUniformBufferParameter<FGaussianSplatShaderResources>(), SplateVF->GetGSplatDataUniformBuffer());
		
	}
};

IMPLEMENT_TYPE_LAYOUT(FGaussianSplatVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FGaussianSplatVertexFactory, SF_Vertex, FGaussianSplatVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FGaussianSplatVertexFactory, SF_Pixel, FGaussianSplatVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FGaussianSplatVertexFactory, SF_Compute, FGaussianSplatVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FGaussianSplatVertexFactory, SF_RayHitGroup, FGaussianSplatVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING

bool FGaussianSplatVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (Parameters.MaterialParameters.bIsMasked || Parameters.MaterialParameters.bIsSpecialEngineMaterial) 
		return true;

	//if (Parameters.MaterialParameters.bIsUsedWithNiagaraSprites)
	//	return true;



	return false;
}

void FGaussianSplatVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
{
	// Check if this vertex factory has a valid feature level that is supported by the current platform
	check(HasValidFeatureLevel());

	//From the Virtual Heightfield Mesh Vertex Factory, we need to have at least one stream to create the declaration, even if it is not used. 

	FVertexStream NullVertexStream;
	NullVertexStream.VertexBuffer = nullptr;
	NullVertexStream.Stride = 0;
	NullVertexStream.Offset = 0;
	NullVertexStream.VertexStreamUsage = EVertexStreamUsage::ManualFetch;

	check(Streams.Num() == 0);
	Streams.Add(NullVertexStream);

	FVertexDeclarationElementList Elements;
	//Elements.Add(FVertexElement(0, 0, VET_None, 0, 0, false));

	
#if 0
	const bool bCanUseGPUScene = UseGPUScene(GMaxRHIShaderPlatform, GMaxRHIFeatureLevel);

	// Add support for primitiveID
	const uint8 Index = static_cast<uint8>(EVertexInputStreamType::Default);
	PrimitiveIdStreamIndex[Index] = -1;
	if (GetType()->SupportsPrimitiveIdStream() && bCanUseGPUScene)
	{
		// When the VF is used for rendering in normal mesh passes, this vertex buffer and offset will be overridden
		Elements.Add(AccessStreamComponent(FVertexStreamComponent(&GPrimitiveIdDummy, 0, 0, sizeof(uint32), VET_UInt, EVertexStreamUsage::Instancing), 13));
		PrimitiveIdStreamIndex[Index] = Elements.Last().StreamIndex;
	}
#endif

	InitDeclaration(Elements);
}
