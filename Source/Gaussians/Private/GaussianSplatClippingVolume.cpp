#include "GaussianSplatClippingVolume.h"

#include "GSWorldSubsystem.h"
#include "Components/BrushComponent.h"


AGaussianSplatClippingVolume::AGaussianSplatClippingVolume()
{
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


FGSClippingVolumeParameters::FGSClippingVolumeParameters(const AGaussianSplatClippingVolume* ClippingVolume)
	: Mode(ClippingVolume->Mode)
	, Priority(ClippingVolume->Priority)
	, Bounds(ClippingVolume->GetBounds().GetBox())
	, Mask(ClippingVolume->Mask)
{
}
