#include "GaussianSplatClippingVolume.h"

#include "GSWorldSubsystem.h"
#include "Components/BrushComponent.h"


AGaussianSplatClippingVolume::AGaussianSplatClippingVolume()
{
	ClippingTags.AddTag(FGameplayTag::RequestGameplayTag("StarLightGS.CullingVolumes.Default", false));

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

#if WITH_EDITOR
void AGaussianSplatClippingVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	MarkClippingStateDirty();
}

void AGaussianSplatClippingVolume::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	MarkClippingStateDirty();
}
#endif

void AGaussianSplatClippingVolume::PopulateClippingVolumeProxy(FGSClippingVolumeProxy& OutProxy) const
{
	OutProxy.Mode = Mode;
	OutProxy.Priority = Priority;

	// Build oriented bounds
	OutProxy.OrientedBounds.Center = GetActorLocation();

	auto Scale = GetActorRelativeScale3D();
	// Convert from scale to extent by multiplying by 100
	OutProxy.OrientedBounds.ExtentX = 100.0f * Scale.X;
	OutProxy.OrientedBounds.ExtentY = 100.0f * Scale.Y;
	OutProxy.OrientedBounds.ExtentZ = 100.0f * Scale.Z;

	OutProxy.OrientedBounds.AxisX = GetActorForwardVector();
	OutProxy.OrientedBounds.AxisY = GetActorRightVector();
	OutProxy.OrientedBounds.AxisZ = GetActorUpVector();

	OutProxy.Tags = ClippingTags;
	OutProxy.bExactMatchOnly = bClippingTagsExactMatchOnly;
}

void AGaussianSplatClippingVolume::MarkClippingStateDirty() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGSWorldSubsystem* WorldSubsystem = World->GetSubsystem<UGSWorldSubsystem>())
		{
			WorldSubsystem->MarkClippingDirtyFlags();
		}
	}
}
