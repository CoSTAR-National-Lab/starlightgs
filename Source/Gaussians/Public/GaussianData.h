#pragma once

#include "CoreMinimal.h"

#include "GaussianData.generated.h"


/**
 * Represents parsed data for a single splat, loaded from a regular PLY file.
 */
USTRUCT()
struct GAUSSIANS_API FGaussianSplatData
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

	// Normal vectors [optional] (nx, ny, nz)
	UPROPERTY()
	TArray<FVector3f> Normals;

	// Spherical Harmonics coefficients [optional] - High order (f_rest_0, ..., f_rest_44)
	UPROPERTY()
	TArray<float> HighOrderHarmonicsCoefficients;

	// PBR properties (metallic, roughness)
	UPROPERTY()
	TArray<FVector2f> MetallicRoughness;


protected:
	// Bounds of all splats in the model. Used for hit proxies, wireframes, culling
	UPROPERTY(VisibleAnywhere)
	FBoxSphereBounds Bounds;


public:

	bool SupportsRelighting() const
	{
		return UseNormals() && UseMetallicRoughness();
	}

public:
	FGaussianSplatData()
	{
		Resize(SplatCount);
	}
	
	FGaussianSplatData(uint32 Num)
	{
		Resize(Num);
	}

	void CopyFrom(const FGaussianSplatData* Other);

	void Empty();
	void Resize(uint32 Num);

	void RecalculateBounds();
	const FBoxSphereBounds& GetBounds() const { return Bounds; }

	static constexpr uint32 NumHighOrderSH = 45;
};
