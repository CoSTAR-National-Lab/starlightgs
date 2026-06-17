#include "GaussianSplatSceneProxy.h"

#include "GaussianSplatComponent.h"
#include "GaussianSplatResource.h"
#include "GaussianSplatVertexFactory.h"

#include "GSWorldSubsystem.h"
#include "GSViewExtension.h"


FGaussianSplatSceneProxy::FGaussianSplatSceneProxy(UGaussianSplatComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, ScaleMultiplier(InComponent->SplatScaleMultiplier)
	, ClippingVolumeMask(InComponent->ClippingVolumeMask)
	, VertexFactory(GMaxRHIFeatureLevel)
{
	if (InComponent->GaussianSplatAsset)
	{
		// Get GPU resources from asset
		// This pointer can be assigned from game thread but NOT dereferenced
		GaussianSplatResource_RenderThread = InComponent->GaussianSplatAsset->GetOrCreateResource();
	}

	ShadowMaterialProxy = InComponent->SplatMaterial ? InComponent->SplatMaterial->GetRenderProxy() : nullptr;

	// Get view extension from world subsystem
	if (UWorld* World = InComponent->GetWorld())
	{
		if (UGSWorldSubsystem* Subsystem = World->GetSubsystem<UGSWorldSubsystem>())
		{
			ViewExtension = Subsystem->GetViewExtension();
		}
	}

	bSupportsRuntimeVirtualTexture = true;
	bIsStaticMesh = false;
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
		VertexFactory.InitResource(RHICmdList);
		VertexFactory.SetGSplatDataUniformBuffer(GaussianSplatResource_RenderThread->GetResource()->GetUniformBuffer());

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
		VertexFactory.ReleaseResource();

		if (auto ViewExt = ViewExtension.Pin())
		{
			ViewExt->UnregisterProxy(this);
		}

		GaussianSplatResource_RenderThread.Reset();
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

FPrimitiveViewRelevance FGaussianSplatSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bRenderCustomDepth = false;
	Result.bRenderInMainPass = View->Family->EngineShowFlags.HitProxies || bDrawVFInMainPass;
	Result.bRenderInDepthPass = false;

	Result.bShadowRelevance = IsShadowCast(View);
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	
	Result.bDynamicRelevance = true;

	return Result;
}

void FGaussianSplatSceneProxy::GetDynamicMeshElements(
	const TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily, 
	uint32 VisibilityMap, 
	FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			if (IsSelected())
			{
				RenderBounds(PDI, ViewFamily.EngineShowFlags, GetBounds(), true);
			}

			if (VertexFactory.IsInitialized() && ShadowMaterialProxy)
			{
				FMeshBatch& Batch = Collector.AllocateMesh();

				Batch.Type = PT_TriangleList;
				Batch.bUseForDepthPass = true;
				Batch.CastShadow = true;
				 
				Batch.CastRayTracedShadow = false;
				Batch.bRenderToVirtualTexture = true;
				Batch.bUseAsOccluder = true;
				Batch.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Batch.bDisableBackfaceCulling = true;
				Batch.DepthPriorityGroup = GetDepthPriorityGroup(Views[ViewIndex]);
				Batch.bCanApplyViewModeOverrides = true;
				Batch.SegmentIndex = 0;

				Batch.MaterialRenderProxy = ShadowMaterialProxy;

				Batch.VertexFactory = &VertexFactory;

				FMeshBatchElement& MeshElement = Batch.Elements[0];
				MeshElement.IndexBuffer = nullptr;
				MeshElement.FirstIndex = 0;
				// One quad per splat -> 2 triangles per quad
				MeshElement.NumPrimitives = 2 * GaussianSplatResource_RenderThread->GetResource()->GetSplatCount();
				MeshElement.NumInstances = 1;
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = 0;

				Collector.AddMesh(ViewIndex, Batch);
			}
		}
	}
}

FGaussianSplatResource* FGaussianSplatSceneProxy::GetResource_RenderThread() const
{
	check(IsInRenderingThread());
	return GaussianSplatResource_RenderThread->GetResource();
}

bool FGaussianSplatSceneProxy::SupportsRelighting() const
{
	check(IsInRenderingThread());
	return GaussianSplatResource_RenderThread->GetResource()->SupportsRelighting();
}
