#include "GaussianSplatSceneProxy.h"

#include <devicetopology.h>

#include "DynamicMeshBuilder.h"
#include "GaussianSplatComponent.h"
#include "GaussianSplatResource.h"
#include "GaussianSplatVertexFactory.h"

#include "GSWorldSubsystem.h"
#include "GSViewExtension.h"
#include "Engine/InstancedStaticMesh.h"



FGaussianSplatSceneProxy::FGaussianSplatSceneProxy(UGaussianSplatComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, ScaleMultiplier(InComponent->SplatScaleMultiplier), VertexFactory(GMaxRHIFeatureLevel)
{
	if (InComponent->GaussianSplatAsset)
	{
		// Get GPU resources from asset
		// This pointer can be assigned from game thread but NOT dereferenced
		GaussianSplatResource_RenderThread = InComponent->GaussianSplatAsset->GetOrCreateResource();
	}

	if(InComponent->SplatMaterial)
	{
		ShadowMaterialProxy = InComponent->SplatMaterial->GetRenderProxy();
	}
	else
	{
		ShadowMaterialProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
		
	}

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
	//check(bCastDynamicShadow)
}

FGaussianSplatSceneProxy::~FGaussianSplatSceneProxy()
{
	// All rendering resources are released in DestroyRenderThreadResources
}

void FGaussianSplatSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
{
	check(IsInRenderingThread());
	FPrimitiveSceneProxy::CreateRenderThreadResources(RHICmdList);

	VertexFactory.InitResource(RHICmdList);

	if (GaussianSplatResource_RenderThread)
	{
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

	//To draw shadow only draw relevance must be off
	// shadow relevance must be IsShadowCast.

	FPrimitiveViewRelevance Result;

	Result.bDrawRelevance = IsShown(View); // View->Family->EngineShowFlags.HitProxies; //IsShown(View); //false;// ; // to draw only shadows
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bRenderInDepthPass = ShouldRenderInDepthPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow; 

	Result.bShadowRelevance = IsShadowCast(View);
	
	Result.bDynamicRelevance = true;

	return Result;
}

struct FGSOneFrameResource : public FOneFrameResource
{
public:
	FGSOneFrameResource(ERHIFeatureLevel::Type InFeatureLevel) : VertexFactory(InFeatureLevel) {};
	~FGSOneFrameResource() override { VertexFactory.ReleaseResource(); }

	FGaussianSplatVertexFactory VertexFactory; 
};

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

			//FBoxSphereBounds Bounds()

			if (IsSelected())
			{
				RenderBounds(PDI, ViewFamily.EngineShowFlags, GetBounds(), true);
			}

			if (VertexFactory.IsInitialized())
			{
				auto& RHI = Collector.GetRHICommandList();
				auto& batch = Collector.AllocateMesh();

				batch.Type = PT_TriangleList;
				batch.bUseForDepthPass = true;
				batch.CastShadow = true;
				 
				batch.CastRayTracedShadow = false;
				batch.bRenderToVirtualTexture = true;
				batch.bUseAsOccluder = true;
				batch.ReverseCulling = IsLocalToWorldDeterminantNegative();
				batch.DepthPriorityGroup = GetDepthPriorityGroup(Views[ViewIndex]);
				batch.bCanApplyViewModeOverrides = true;
				batch.SegmentIndex = 0;

				batch.MaterialRenderProxy = ShadowMaterialProxy;
				//batch.PreparePrimitiveUniformBuffer(this, GMaxRHIFeatureLevel);

				//auto& VertexFactoryPTR = Collector.AllocateOneFrameResource<FGSOneFrameResource>(Views[ViewIndex]->GetFeatureLevel());
				 
				//VertexFactoryPTR.VertexFactory.InitResource(RHI);
				//VertexFactoryPTR.VertexFactory.SetGSplatDataUniformBuffer(GaussianSplatResource_RenderThread->GetResource()->GetUniformBuffer());

				batch.VertexFactory = &VertexFactory;//&VertexFactoryPTR.VertexFactory;

				FMeshBatchElement& MeshElement = batch.Elements[0];

				MeshElement.IndexBuffer = nullptr;
				MeshElement.FirstIndex = 0;
				MeshElement.NumPrimitives = 2 * 100;
				MeshElement.NumInstances = 1;
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = 0;
				
				//MeshElement.PrimitiveUniformBuffer = GetUniformBuffer(); // we use primitive id stream and fetch primitive using GPUScene

				Collector.AddMesh(ViewIndex, batch);
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
				const uint32 V000 = MeshBuilder.AddVertex(FVector3f(Min.X, Min.Y, Min.Z), UV, TX, TY, TZ, White);
				const uint32 V100 = MeshBuilder.AddVertex(FVector3f(Max.X, Min.Y, Min.Z), UV, TX, TY, TZ, White);
				const uint32 V010 = MeshBuilder.AddVertex(FVector3f(Min.X, Max.Y, Min.Z), UV, TX, TY, TZ, White);
				const uint32 V110 = MeshBuilder.AddVertex(FVector3f(Max.X, Max.Y, Min.Z), UV, TX, TY, TZ, White);
				const uint32 V001 = MeshBuilder.AddVertex(FVector3f(Min.X, Min.Y, Max.Z), UV, TX, TY, TZ, White);
				const uint32 V101 = MeshBuilder.AddVertex(FVector3f(Max.X, Min.Y, Max.Z), UV, TX, TY, TZ, White);
				const uint32 V011 = MeshBuilder.AddVertex(FVector3f(Min.X, Max.Y, Max.Z), UV, TX, TY, TZ, White);
				const uint32 V111 = MeshBuilder.AddVertex(FVector3f(Max.X, Max.Y, Max.Z), UV, TX, TY, TZ, White);


				TArray<uint32> Indices =
				{
					V000, V010, V100,
					V010, V110, V100,

					V001, V101, V011,
					V101, V111, V011,

					V000, V100, V001,
					V100, V101, V001,

					V010, V011, V110,
					V011, V111, V110,

					V000, V001, V010,
					V001, V011, V010,

					V100, V110, V101,
					V110, V111, V101,
				};

				MeshBuilder.AddTriangles(Indices);

				// bDisableBackfaceCulling=true so all faces are drawn regardless of winding.
				UMaterialInterface* HitMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
				MeshBuilder.GetMesh(
					GetLocalToWorld(),
					HitMaterial->GetRenderProxy(),
					SDPG_World,
					/*bDisableBackfaceCulling=*/true,
					/*bReceivesDecals=*/true,
					/*bUseSelectionOutline=*/true,
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

void FGaussianSplatSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	//if (GaussianSplatResource_RenderThread)
	//{
		//PDI->ReserveMemoryForMeshes(1);

		//FMeshBatch batch{};

		//batch.Type = PT_TriangleStrip;
		//batch.bUseForDepthPass = true;
		//batch.CastShadow = true;
		//batch.CastRayTracedShadow = false;
		//batch.bUseAsOccluder = false;
		//batch.ReverseCulling = false; // IsLocalToWorldDeterminantNegative();
		//batch.DepthPriorityGroup = ESceneDepthPriorityGroup::SDPG_World;
		//batch.bCanApplyViewModeOverrides = true;
		//batch.SegmentIndex = 0;
		//batch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(); // the default one, otherwise should be the render proxy from the asset;


		//batch.VertexFactory = &VertexFactory;

		//FMeshBatchElement& MeshElement = batch.Elements[0];

		//MeshElement.IndexBuffer = nullptr;
		//MeshElement.FirstIndex = 0;
		//MeshElement.NumPrimitives = 2;
		//MeshElement.NumInstances = 1;
		//MeshElement.MinVertexIndex = 0;
		//MeshElement.MaxVertexIndex = 0;
		//MeshElement.PrimitiveUniformBuffer = GetUniformBuffer();

		//PDI->DrawMesh(batch, FLT_MAX);
	//}

	FPrimitiveSceneProxy::DrawStaticElements(PDI);
}
