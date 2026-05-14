#include "GaussianSplatComponent.h"

#include "GaussianSplatSceneProxy.h"


#if WITH_EDITOR
void UGaussianSplatComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

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
