#pragma once

#include "CoreMinimal.h"

#include "FrostingData.generated.h"


USTRUCT(BlueprintType)
struct FrostingBaseMeshData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FVector2D> InnerOuterPrismDistance;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<int32> Indices;

};

USTRUCT(BlueprintType)

struct FPrismBarycentric
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float a0 = 0;
	UPROPERTY(EditAnywhere)
	float a1 = 0;
	UPROPERTY(EditAnywhere)
	float a2 = 0;
	UPROPERTY(EditAnywhere)
	float a3 = 0;
	UPROPERTY(EditAnywhere)
	float a4 = 0;
	UPROPERTY(EditAnywhere)
	float a5 = 0;

	float GetSeventhCoord()
	{
		return 1.f - (a0 + a1 + a2 + a3 + a4 + a5);
	}
};

USTRUCT(BlueprintType)

struct FGaussianFrostingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<FPrismBarycentric> BarryCoords;

	UPROPERTY(EditAnywhere)
	TArray<int32> FaceIndices;

	UPROPERTY(EditAnywhere)
	TArray<FVector3f> Scales;

	UPROPERTY(EditAnywhere)
	TArray<FQuat4f> Orientations;

	UPROPERTY(EditAnywhere)
	TArray<FVector4f> SH0AndOpacities;

	void Resize(uint32 NewNum)
	{
		SplatCount = NewNum;
		FaceIndices.Init(0, NewNum);
		BarryCoords.Init(FPrismBarycentric(), NewNum);
		Scales.Init(FVector3f(), NewNum);
		Orientations.Init(FQuat4f(), NewNum);
		SH0AndOpacities.Init(FVector4f(), NewNum);
	}

	void Empty()
	{
		SplatCount = 0;
		FaceIndices.Empty();
		BarryCoords.Empty();
		Scales.Empty();
		Orientations.Empty();
		SH0AndOpacities.Empty();
	}

	void CopyFrom(const FGaussianFrostingData* Other)
	{
		SplatCount = Other->SplatCount;
		FaceIndices = Other->FaceIndices;
		BarryCoords = Other->BarryCoords;
		//Normals = Other->Normals;
		Orientations = Other->Orientations;
		Scales = Other->Scales;
		SH0AndOpacities = Other->SH0AndOpacities;
		//HighOrderHarmonicsCoefficients = Other->HighOrderHarmonicsCoefficients;
	}

	uint32 GetSplatCount() const { return SplatCount; }

	uint32 SplatCount;
};

USTRUCT(BlueprintType)
struct FFrostingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FrostingBaseMeshData Mesh;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FGaussianFrostingData Splats;
};
