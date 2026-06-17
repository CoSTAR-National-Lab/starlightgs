#include "GaussianSplatComponent.h"

#include "GaussianSplatSceneProxy.h"


#if WITH_EDITOR
void UGaussianSplatComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Stops from crashing when adding splat as component in blueprint
	// TODO: Splat doesn't render in blueprint viewport though
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsValid(this))
	{
		return;
	}

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UGaussianSplatComponent, GaussianSplatAsset))
	{
		if (PrevGaussianSplatAsset)
		{
			PrevGaussianSplatAsset->OnAssetUpdate.RemoveDynamic(this, &UGaussianSplatComponent::OnAssetUpdated);
		}
		PrevGaussianSplatAsset = GaussianSplatAsset;
		if (GaussianSplatAsset)
		{
			GaussianSplatAsset->OnAssetUpdate.AddDynamic(this, &UGaussianSplatComponent::OnAssetUpdated);
		}
	}
}
#endif

void UGaussianSplatComponent::OnRegister()
{
	Super::OnRegister();

	PrevGaussianSplatAsset = GaussianSplatAsset;
	if (GaussianSplatAsset)
	{
		GaussianSplatAsset->OnAssetUpdate.AddDynamic(this, &UGaussianSplatComponent::OnAssetUpdated);
	}
}

void UGaussianSplatComponent::OnUnregister()
{
	Super::OnUnregister();

	if (GaussianSplatAsset)
	{
		GaussianSplatAsset->OnAssetUpdate.RemoveDynamic(this, &UGaussianSplatComponent::OnAssetUpdated);
	}
}

FPrimitiveSceneProxy* UGaussianSplatComponent::CreateSceneProxy()
{
	bCastDynamicShadow = true;
	//GetUsedMaterials(OuMaterials)



	return new FGaussianSplatSceneProxy(this);
}

FBoxSphereBounds UGaussianSplatComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GaussianSplatAsset)
	{
		FBoxSphereBounds LocalBox = GaussianSplatAsset->Splats.GetBounds();

		// Transform to world space
		FBoxSphereBounds WorldBox = LocalBox.TransformBy(LocalToWorld);

		return FBoxSphereBounds(WorldBox);
	}

	return FBoxSphereBounds{ };
}

void UGaussianSplatComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
	
	//register material used for splats if any, this will propagate to proxy and permute shader with custom vertex factory.
	if (SplatMaterial)
		OutMaterials.Add(SplatMaterial);
}

void UGaussianSplatComponent::OnAssetUpdated()
{
	MarkRenderStateDirty();
}


AGaussianSplatActor::AGaussianSplatActor(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	GaussianSplatComponent = CreateDefaultSubobject<UGaussianSplatComponent>(TEXT("GaussianSplatComponent"));
	RootComponent = GaussianSplatComponent;
}
