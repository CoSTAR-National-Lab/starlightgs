#include "GaussianSplatClippingVolume.h"

#include "GSWorldSubsystem.h"
#include "Components/BrushComponent.h"


AGaussianSplatClippingVolume::AGaussianSplatClippingVolume()
{
	MaskTags.AddTag(FGameplayTag::RequestGameplayTag("StarLightGS.CullingVolumes.Default", false));

	bColored = true;
	BrushColor.R = 0;
	BrushColor.G = 128;
	BrushColor.B = 128;
	BrushColor.A = 255;

	GetBrushComponent()->SetMobility(EComponentMobility::Movable);
}


void AGaussianSplatClippingVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (UWorld* World = GetWorld())
	{
		if (UGSWorldSubsystem* WorldSubsystem = World->GetSubsystem<UGSWorldSubsystem>())
		{
			WorldSubsystem->AddClippingVolume(this);
		}
	}
}

void AGaussianSplatClippingVolume::BeginDestroy()
{
	Super::BeginDestroy();

	if (UWorld* World = GetWorld())
	{
		if (UGSWorldSubsystem* WorldSubsystem = World->GetSubsystem<UGSWorldSubsystem>())
		{
			WorldSubsystem->RemoveClippingVolume(this);
		}
	}
}

void AGaussianSplatClippingVolume::PopulateClippingVolumeProxy(FGSClippingVolumeProxy& OutProxy) const
{
	OutProxy.Mode = Mode;
	OutProxy.Priority = Priority;
	OutProxy.Bounds = GetBounds().GetBox();
	OutProxy.Mask = Mask;
}
