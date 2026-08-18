#include "GaussianData.h"


void FGaussianSplatData::CopyFrom(const FGaussianSplatData* Other)
{
	SplatCount = Other->SplatCount;

	Positions = Other->Positions;
	Orientations = Other->Orientations;
	Scales = Other->Scales;
	ZeroOrderHarmonicsCoefficientsAndOpacities = Other->ZeroOrderHarmonicsCoefficientsAndOpacities;

	Normals = Other->Normals;
	HighOrderHarmonicsCoefficients = Other->HighOrderHarmonicsCoefficients;
	MetallicRoughness = Other->MetallicRoughness;
}

void FGaussianSplatData::Empty()
{
	SplatCount = 0;

	Positions.Empty();
	Orientations.Empty();
	Scales.Empty();
	ZeroOrderHarmonicsCoefficientsAndOpacities.Empty();

	Normals.Empty();
	HighOrderHarmonicsCoefficients.Empty();
	MetallicRoughness.Empty();
}

void FGaussianSplatData::Resize(uint32 Num)
{
	SplatCount = Num;

	Positions.Init(FVector4f(0, 0, 0, 0), SplatCount);
	Orientations.Init(FQuat4f::Identity, SplatCount);
	Scales.Init(FVector4f(0, 0, 0, 0), SplatCount);
	ZeroOrderHarmonicsCoefficientsAndOpacities.Init(FVector4f(0, 0, 0, 0), SplatCount);

	if (bHasNormals)
	{
		Normals.Init(FVector4f(0, 0, 0, 0), SplatCount);
	}

	if (bHasHighOrderSH)
	{
		HighOrderHarmonicsCoefficients.Init(0.f, SplatCount * NumHighOrderSH);
	}

	if (bHasMetallicRoughness)
	{
		MetallicRoughness.Init(FVector2f(0, 1), SplatCount);
	}
}

void FGaussianSplatData::RecalculateBounds()
{
	FBoxSphereBounds::Builder Builder{ };

	for (const auto& Point : Positions)
	{
		// TODO: Scale point and add min/max to take into account size of splat
		Builder += static_cast<FVector>(Point);
	}

	Bounds = Builder;
}
