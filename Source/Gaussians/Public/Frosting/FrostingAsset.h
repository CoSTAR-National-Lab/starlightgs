#pragma once

#include "FrostingData.h"
#include "CoreMinimal.h"

#include "FrostingAsset.generated.h"


UCLASS(BlueprintType)
class GAUSSIANS_API UFrostingDataAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly);
	FFrostingData Frosting;

	UPROPERTY(VisibleAnywhere)
	int TriangleCount;

	UPROPERTY(VisibleAnywhere)
	int  VertexCount;

	UPROPERTY(VisibleAnywhere)
	int SplatCount;

	UFUNCTION()
	void UpdateAssetCounters()
	{
		SplatCount = Frosting.Splats.GetSplatCount();
		VertexCount = Frosting.Mesh.Vertices.Num();
		TriangleCount = Frosting.Mesh.Indices.Num() / 3;
	}

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;
#endif
};
