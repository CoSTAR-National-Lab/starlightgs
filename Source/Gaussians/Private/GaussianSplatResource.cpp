#include "GaussianSplatResource.h"

#include "GaussianData.h"


IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGaussianSplatShaderResources, "GSResources")


FGaussianSplatResource::FGaussianSplatResource(FGaussianSplatData InSplatData)
	: SplatData(MoveTemp(InSplatData))
{
}

FGaussianSplatResource::~FGaussianSplatResource()
{
	// All RHI resources should have already been released by calling ReleaseRHI
}

void FGaussianSplatResource::InitRHI(FRHICommandListBase& RHICmdList)
{
	check(IsInRenderingThread());

	// Do not create resources if there are no splats
	if (SplatData.SplatCount == 0)
	{
		return;
	}

	// Ensure all required fields are there
	check(
		SplatData.SplatCount == SplatData.Positions.Num() &&
		SplatData.SplatCount == SplatData.Scales.Num() &&
		SplatData.SplatCount == SplatData.Orientations.Num() &&
		SplatData.SplatCount == SplatData.ZeroOrderHarmonicsCoefficientsAndOpacities.Num()
	)

	// If optional fields are declared, then ensure there is the correct quantity
	check(!SplatData.UseNormals() || SplatData.SplatCount == SplatData.Normals.Num());
	check(!SplatData.UseHighOrderSH() || SplatData.SplatCount * FGaussianSplatData::NumHighOrderSH == SplatData.HighOrderHarmonicsCoefficients.Num());
	check(!SplatData.UseMetallicRoughness() || SplatData.SplatCount == SplatData.MetallicRoughness.Num());

	// Allocates, populates, and creates an SRV for a given array.
	// If Predicate is false, then a dummy buffer with no data will be created instead
	auto CreateBuffer = [&]<typename T>(const TArray<T>& Source, const TCHAR* DebugName, bool Predicate=true)
	{
		uint32 BytesPerElement = Source.GetTypeSize();
		uint32 NumElements = Source.Num();

		FBufferRHIRef Buffer = nullptr;
		FShaderResourceViewRHIRef SRV = nullptr;

		if (NumElements > 0 && Predicate)
		{
			const FRHIBufferCreateDesc CreateDesc =
				FRHIBufferCreateDesc::CreateStructured(DebugName, BytesPerElement * NumElements, BytesPerElement)
				.AddUsage(EBufferUsageFlags::ShaderResource)
				.DetermineInitialState()
				.SetInitActionInitializer();

			FRHIBufferInitializer Initializer = RHICmdList.CreateBufferInitializer(CreateDesc);
			Initializer.WriteData(Source.GetData(), BytesPerElement * NumElements);
			Buffer = Initializer.Finalize();
		}
		else
		{
			// Creates a single-element buffer to allow for a valid SRV to be placed into the uniform buffer
			// Unreal will validate that all resources in the UB are valid
			// We cannot use GSystemTextures here because we do not have a RDGBuilder
			const FRHIBufferCreateDesc CreateDesc =
				FRHIBufferCreateDesc::CreateStructured(DebugName, BytesPerElement, BytesPerElement)
				.AddUsage(EBufferUsageFlags::ShaderResource)
				.DetermineInitialState();
			Buffer = RHICmdList.CreateBuffer(CreateDesc);
		}

		SRV = RHICmdList.CreateShaderResourceView(
			Buffer,
			FRHIViewDesc::CreateBufferSRV()
			.SetType(FRHIViewDesc::EBufferType::Structured)
			.SetStride(BytesPerElement));

		return FBufferResources{ Buffer, SRV };
	};

	PositionsBuffer = CreateBuffer(SplatData.Positions, TEXT("GaussianSplatsPositions"));
	ScalesBuffer = CreateBuffer(SplatData.Scales, TEXT("GaussianSplatsScales"));
	OrientationsBuffer = CreateBuffer(SplatData.Orientations, TEXT("GaussianSplatsOrientations"));
	SHAndOpacitiesBuffer = CreateBuffer(SplatData.ZeroOrderHarmonicsCoefficientsAndOpacities, TEXT("GaussianSplatsSHAndOpacities"));

	// Will be a buffer with a single element if splat data does not use property
	// Uniform buffers do not allow null SRVs, has to point to a valid resource
	NormalsBuffer = CreateBuffer(SplatData.Normals, TEXT("GaussianSplatsNormals"), SplatData.UseNormals());
	HighOrderSHBuffer = CreateBuffer(SplatData.HighOrderHarmonicsCoefficients, TEXT("GaussianSplatHighOrderSH"), SplatData.UseHighOrderSH());
	MetallicRoughnessBuffer = CreateBuffer(SplatData.MetallicRoughness, TEXT("GaussianSplatMetallicRoughness"), SplatData.UseMetallicRoughness());

	// Create uniform buffer
	FGaussianSplatShaderResources Parameters;
	Parameters.SplatCount = SplatData.SplatCount;

	Parameters.Positions = PositionsBuffer.SRV;
	Parameters.Scales = ScalesBuffer.SRV;
	Parameters.Orientations = OrientationsBuffer.SRV;
	Parameters.SHAndOpacities = SHAndOpacitiesBuffer.SRV;

	Parameters.Normals = NormalsBuffer.SRV;
	Parameters.HighOrderSH = HighOrderSHBuffer.SRV;
	Parameters.MetallicRoughness = MetallicRoughnessBuffer.SRV;

	UniformBuffer = TUniformBufferRef<FGaussianSplatShaderResources>::CreateUniformBufferImmediate(Parameters, UniformBuffer_MultiFrame);
}

void FGaussianSplatResource::ReleaseRHI()
{
	check(IsInRenderingThread());

	UniformBuffer.SafeRelease();

	PositionsBuffer.SafeRelease();
	ScalesBuffer.SafeRelease();
	OrientationsBuffer.SafeRelease();
	SHAndOpacitiesBuffer.SafeRelease();

	NormalsBuffer.SafeRelease();
	HighOrderSHBuffer.SafeRelease();
	MetallicRoughnessBuffer.SafeRelease();
}

void FGaussianSplatResource::UpdateSplats(FRHICommandListBase& RHICmdList, FGaussianSplatData InSplatData)
{
	check(IsInRenderingThread());

	SplatData = MoveTemp(InSplatData);
	UpdateRHI(RHICmdList);
}
