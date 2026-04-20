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