#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

#include "GSplatAsset.h"

#include "GaussianSplatComponent.generated.h"


UCLASS(Blueprintable, ClassGroup = (Rendering), hidecategories = (Collision, Physics, Navigation), meta=(BlueprintSpawnableComponent))
class GAUSSIANS_API UGaussianSplatComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:

	//  TOD O: Need custom setter to make it BP-RW
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gaussian Splats")
	TObjectPtr<UGSplatAsset> GaussianSplatAsset = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gaussian Splats")
	float SplatScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gaussian Splats")
	UMaterial* SplatMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gaussian Splats")
	int32 ClippingVolumeMask = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gaussian Splats", AdvancedDisplay)
	bool bForceDrawVertexFactoryInMainPass = false;


public:

	UGaussianSplatComponent();

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent Interface

	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent Interface

	UFUNCTION(Category="Default", CallInEditor)
	void ExportClippedSplat();

private:
	UFUNCTION()
	void OnAssetUpdated();

private:
	TObjectPtr<UGSplatAsset> PrevGaussianSplatAsset = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* SpriteTexture;
#endif
};


UCLASS(Blueprintable)
class GAUSSIANS_API AGaussianSplatActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Gaussian Splats", meta=(AllowPrivateAccess=true))
	TObjectPtr<UGaussianSplatComponent> GaussianSplatComponent;
};
