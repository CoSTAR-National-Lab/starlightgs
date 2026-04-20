#pragma once

#include "CoreMinimal.h"

#include "GaussianData.generated.h"


/**
 * Represents parsed data for a single splat, loaded from a regular PLY file.
 */
USTRUCT()
struct FGaussianSplatData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint32 SplatCount = 0;

	// Required properties:

	// Splat position (x, y, z)
	UPROPERTY()
	TArray<FVector3f> Positions;

	// Splat orientation coming as wxyz from PLY (rot_0, rot_1, rot_2, rot_3)
	UPROPERTY()
	TArray<FQuat4f> Orientations;

	// Splat scale (scale_0, scale_1, scale_2)
	UPROPERTY()
	TArray<FVector3f> Scales;

	// Spherical Harmonics coefficients - Zero order (f_dc_0, f_dc_1, f_dc_2)
	UPROPERTY()
	TArray<FVector4f> ZeroOrderHarmonicsCoefficientsAndOpacities;


	// Optional properties:

	UPROPERTY(VisibleAnywhere)
	bool bHasNormals = false;

	UPROPERTY(EditAnywhere, meta=(EditCondition="bHasNormals", ToolTip="If normals are available, overrides whether normals will be used for rendering or not."))
	bool bUseNormals = true;

	UPROPERTY(VisibleAnywhere)
	bool bHasHighOrderSH = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHasNormals", ToolTip = "If higher-order SH are available, overrides whether higher-order SH will be used for rendering or not."))
	bool bUseHighOrderSH = true;

	UPROPERTY(VisibleAnywhere)
	bool bHasMetallicRoughness = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHasNormals", ToolTip = "If Metallic/Roughness is available, overrides whether Metallic/Roughness will be used for rendering or not."))
	bool bUseMetallicRoughness = true;

	bool UseNormals() const { return bHasNormals && bUseNormals; }
	bool UseHighOrderSH() const { return bHasHighOrderSH && bUseHighOrderSH; }
	bool UseMetallicRoughness() const { return bHasMetallicRoughness && bUseMetallicRoughness; }

public:
	// Normal vectors [optional] (nx, ny, nz)
	UPROPERTY()
	TArray<FVector3f> Normals;

	// Spherical Harmonics coefficients [optional] - High order (f_rest_0, ..., f_rest_44)
	UPROPERTY()
	TArray<float> HighOrderHarmonicsCoefficients;

	// PBR properties (metallic, roughness)
	UPROPERTY()
	TArray<FVector2f> MetallicRoughness;

	FGaussianSplatData()
	{
		Resize(SplatCount);
	}
	
	FGaussianSplatData(uint32 Num)
	{
		Resize(Num);
	}

	void CopyFrom(const FGaussianSplatData* Other)
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

	void Empty()
	{
		SplatCount = 0;

		Positions									.Empty();
		Orientations 								.Empty();
		Scales 										.Empty();
		ZeroOrderHarmonicsCoefficientsAndOpacities 	.Empty();

		Normals 									.Empty();
		HighOrderHarmonicsCoefficients 				.Empty();
		MetallicRoughness.Empty();
	}

	void Resize(uint32 Num)
	{
		SplatCount = Num;

		Positions.Init(FVector4f(0, 0, 0, 0),  SplatCount);
		Orientations.Init(FQuat4f::Identity,  SplatCount);
		Scales.Init(FVector4f(0, 0, 0, 0),  SplatCount);
		ZeroOrderHarmonicsCoefficientsAndOpacities.Init(FVector4f(0, 0, 0, 0), SplatCount);

		if (bHasNormals)
		{
			Normals.Init(FVector4f(0, 0, 0, 0),  SplatCount);
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


	static constexpr uint32 NumHighOrderSH = 45;
};
