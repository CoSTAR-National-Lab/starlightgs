#include "GSWorldSubsystem.h"
#include "GSViewExtension.h"

void UGSWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GaussianRendererViewExtension = FSceneViewExtensions::NewExtension<FGSViewExtension>(this);
}

void UGSWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UGSWorldSubsystem::AddClippingVolume(const AGaussianSplatClippingVolume* ClippingVolume)
{
	check(IsInGameThread() || IsInParallelGameThread());

	if (!ClippingVolumes.Contains(ClippingVolume))
	{
		ClippingVolumes.Add(ClippingVolume);
	}
}

void UGSWorldSubsystem::RemoveClippingVolume(const AGaussianSplatClippingVolume* ClippingVolume)
{
	check(IsInGameThread() || IsInParallelGameThread());
	check(ClippingVolumes.Contains(ClippingVolume));

	ClippingVolumes.Remove(ClippingVolume);
}

const TArray<const AGaussianSplatClippingVolume*>& UGSWorldSubsystem::GetClippingVolumes()
{
	ClippingVolumes.RemoveAll([](const auto& Element){ return Element == nullptr; });
	return ClippingVolumes;
}
