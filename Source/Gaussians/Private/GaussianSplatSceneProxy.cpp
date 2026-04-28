#include "GaussianSplatSceneProxy.h"

#include "DynamicMeshBuilder.h"
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
		if (auto ViewExt = ViewExtension.Pin())
		{
			ViewExt->UnregisterProxy(this);
		}

		GaussianSplatResource_RenderThread.Reset();
	}
}

#if WITH_EDITOR
HHitProxy* FGaussianSplatSceneProxy::CreateHitProxies(UPrimitiveComponent* Component,
	TArray<TRefCountPtr<HHitProxy>>& OutHitProxies)
{
	HHitProxy* DefaultProxy = FPrimitiveSceneProxy::CreateHitProxies(Component, OutHitProxies);
	SelectionHitProxy = DefaultProxy;
	return DefaultProxy;
}
#endif

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
	Result.bShadowRelevance = false;
	Result.bDynamicRelevance = true;
	Result.bStaticRelevance = false;
	Result.bRenderInMainPass = true;
	Result.bUsesLightingChannels = false;
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();

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

			// Hit proxy building taken from NanoGS:
			// https://github.com/TimChen1383/NanoGaussianSplatting/blob/main/Plugins/NanoGS/Source/NanoGS/Private/GaussianSplatSceneProxy.cpp
#if WITH_EDITOR
			if (ViewFamily.EngineShowFlags.HitProxies)
			{
				const FBox LocalBox = GetLocalBounds().GetBox();
				const FVector3f Min(LocalBox.Min);
				const FVector3f Max(LocalBox.Max);

				FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());

				// Tangent basis (arbitrary – not shaded, just needs to be valid)
				const FVector2f UV(0.f, 0.f);
				const FVector3f TX(1.f, 0.f, 0.f);
				const FVector3f TY(0.f, 1.f, 0.f);
				const FVector3f TZ(0.f, 0.f, 1.f);
				const FColor White = FColor::White;

				// 8 corners  (named by which axes are at Max: 0=Min, 1=Max)
				const int32 V000 = MeshBuilder.AddVertex(FVector3f(Min.X, Min.Y, Min.Z), UV, TX, TY, TZ, White);
				const int32 V100 = MeshBuilder.AddVertex(FVector3f(Max.X, Min.Y, Min.Z), UV, TX, TY, TZ, White);
				const int32 V010 = MeshBuilder.AddVertex(FVector3f(Min.X, Max.Y, Min.Z), UV, TX, TY, TZ, White);
				const int32 V110 = MeshBuilder.AddVertex(FVector3f(Max.X, Max.Y, Min.Z), UV, TX, TY, TZ, White);
				const int32 V001 = MeshBuilder.AddVertex(FVector3f(Min.X, Min.Y, Max.Z), UV, TX, TY, TZ, White);
				const int32 V101 = MeshBuilder.AddVertex(FVector3f(Max.X, Min.Y, Max.Z), UV, TX, TY, TZ, White);
				const int32 V011 = MeshBuilder.AddVertex(FVector3f(Min.X, Max.Y, Max.Z), UV, TX, TY, TZ, White);
				const int32 V111 = MeshBuilder.AddVertex(FVector3f(Max.X, Max.Y, Max.Z), UV, TX, TY, TZ, White);

				// -Z face
				MeshBuilder.AddTriangle(V000, V010, V100);
				MeshBuilder.AddTriangle(V010, V110, V100);
				// +Z face
				MeshBuilder.AddTriangle(V001, V101, V011);
				MeshBuilder.AddTriangle(V101, V111, V011);
				// -Y face
				MeshBuilder.AddTriangle(V000, V100, V001);
				MeshBuilder.AddTriangle(V100, V101, V001);
				// +Y face
				MeshBuilder.AddTriangle(V010, V011, V110);
				MeshBuilder.AddTriangle(V011, V111, V110);
				// -X face
				MeshBuilder.AddTriangle(V000, V001, V010);
				MeshBuilder.AddTriangle(V001, V011, V010);
				// +X face
				MeshBuilder.AddTriangle(V100, V110, V101);
				MeshBuilder.AddTriangle(V110, V111, V101);

				// Use default opaque surface material – only its depth output matters here.
				// bDisableBackfaceCulling=true so all faces are drawn regardless of winding.
				UMaterialInterface* HitMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
				MeshBuilder.GetMesh(
					GetLocalToWorld(),
					HitMaterial->GetRenderProxy(),
					SDPG_World,
					/*bDisableBackfaceCulling=*/true,
					/*bReceivesDecals=*/false,
					/*bUseSelectionOutline=*/false,
					ViewIndex,
					Collector,
					SelectionHitProxy);
			}
#endif // WITH_EDITOR
		}
	}
}

FGaussianSplatResource* FGaussianSplatSceneProxy::GetResource_RenderThread() const
{
	return GaussianSplatResource_RenderThread->GetResource();
}

bool FGaussianSplatSceneProxy::SupportsRelighting() const
{
	return GaussianSplatResource_RenderThread->GetResource()->SupportsRelighting();
}
