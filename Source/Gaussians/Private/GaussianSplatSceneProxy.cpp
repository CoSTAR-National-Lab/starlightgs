#include "GaussianSplatSceneProxy.h"

#include "GaussianSplatComponent.h"
#include "GaussianSplatResource.h"

#include "GSWorldSubsystem.h"
#include "GSViewExtension.h"


FGaussianSplatSceneProxy::FGaussianSplatSceneProxy(UGaussianSplatComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	if (InComponent->GaussianSplatAsset)
	{
		// Get GPU resources from asset
		// This pointer can be assigned from game thread but NOT dereferenced
		GaussianSplatResource_RenderThread = InComponent->GaussianSplatAsset->GetOrCreateResource();
	}

	// Get view extension from world subsystem
	if (UWorld* World = InComponent->GetWorld())
	{
		if (UGSWorldSubsystem* Subsystem = World->GetSubsystem<UGSWorldSubsystem>())
		{
			ViewExtension = Subsystem->GetViewExtension();
		}
	}
}

FGaussianSplatSceneProxy::~FGaussianSplatSceneProxy()
{
	// All rendering resources are released in DestroyRenderThreadResources
}

void FGaussianSplatSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
{
	check(IsInRenderingThread());
	FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);

	if (GaussianSplatResource_RenderThread)
	{
		if (auto ViewExt = ViewExtension.Pin())
		{
			ViewExt->RegisterProxy(this);
		}
	}
}

void FGaussianSplatSceneProxy::DestroyRenderThreadResources()
{
	check(IsInRenderingThread());
	FPrimitiveSceneProxy::DestroyRenderThreadResources();

	if (GaussianSplatResource_RenderThread)
	{
		GaussianSplatResource_RenderThread.Reset();

		if (auto ViewExt = ViewExtension.Pin())
		{
			ViewExt->UnregisterProxy(this);
		}
	}
}

SIZE_T FGaussianSplatSceneProxy::GetTypeHash() const
{
	static SIZE_T UniquePtr;
	return reinterpret_cast<SIZE_T>(&UniquePtr);
}

uint32 FGaussianSplatSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

FGaussianSplatResource* FGaussianSplatSceneProxy::GetResource_RenderThread() const
{
	return GaussianSplatResource_RenderThread->GetResource();
}
