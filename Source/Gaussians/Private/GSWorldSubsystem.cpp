#include "GSWorldSubsystem.h"
#include "GSViewExtension.h"

#include "GaussianSplatComponent.h"
#include "GaussianSplatSceneProxy.h"

#include "RenderGraphBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rendering/GSExportPass.h"


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

void UGSWorldSubsystem::ExportClippedSplat(UGaussianSplatComponent* Component)
{
	FGaussianSplatSceneProxy* SceneProxy = static_cast<FGaussianSplatSceneProxy*>(Component->GetSceneProxy());

	TArray<FVector3f> Positions;
	TArray<FVector3f> Scales;
	TArray<FQuat4f>   Orientations;
	TArray<FVector4f> SHAndOpacities;
	TArray<FVector3f> Normals;
	TArray<FVector2f> MetallicRoughness;

	
	ENQUEUE_RENDER_COMMAND(GaussiansExportClippedSplat)([&Positions, &Scales, &Orientations, &SHAndOpacities, &Normals, &MetallicRoughness, SceneProxy]
	(FRHICommandListImmediate& RHICmdList)
	{
		bool bRelighting = SceneProxy->SupportsRelighting();

		FRDGBuilder GraphBuilder(RHICmdList);
		FGaussianSplatReadbackData ReadbackData;

		Gaussians::ExportSplatData(
			GraphBuilder,
			SceneProxy,
			ReadbackData
		);

		GraphBuilder.Execute();

		// Make sure work is submitted to the GPU immediately so that we can wait on it
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread, ERHISubmitFlags::SubmitToGPU | ERHISubmitFlags::FlushRHIThread);

		// Wait for work to complete on the GPU
		ReadbackData.Fence->Wait(RHICmdList, RHICmdList.GetGPUMask());

		// Now take the data from the buffers to populate CPU arrays that will form the asset
		// First read the splat count to know how big to make the arrays
		uint32 SplatCount = *static_cast<uint32*>(RHILockStagingBuffer(ReadbackData.SplatCount, 0, sizeof(uint32)));
		RHIUnlockStagingBuffer(ReadbackData.SplatCount);
		
		auto DoCopyIntoBuffer = [&]<typename T>(FRHIStagingBuffer* StagingBuffer, TArray<T>& DestArray)
		{
			DestArray.SetNumUninitialized(SplatCount);

			void* DataPtr = RHILockStagingBuffer(StagingBuffer, 0, SplatCount * sizeof(T));
			FMemory::Memcpy(DestArray.GetData(), DataPtr, SplatCount * sizeof(T));
			RHIUnlockStagingBuffer(StagingBuffer);
		};

		DoCopyIntoBuffer(ReadbackData.Positions, Positions);
		DoCopyIntoBuffer(ReadbackData.Scales, Scales);
		DoCopyIntoBuffer(ReadbackData.Orientations, Orientations);
		DoCopyIntoBuffer(ReadbackData.SHAndOpacities, SHAndOpacities);
		if (bRelighting)
		{
			DoCopyIntoBuffer(ReadbackData.Normals, Normals);
			DoCopyIntoBuffer(ReadbackData.MetallicRoughness, MetallicRoughness);
		}
	});

	FlushRenderingCommands();

	// Data is now ready to use to populate asset
	// TODO: I want this to work in the runtime module, not editor module, since this is related to the world...
	// TODO: In the future it would be better to trigger this from the editor module, then this can hand back off to the editor module for asset creation
	const FString AssetNameSuffix = Component->GetOwner()->GetName() + "_Exported";

	const FString BasePackageName = TEXT("/Game/Splats/");
	const FString PackagePath = FPackageName::GetLongPackagePath(BasePackageName);

	const FString AssetName = FPackageName::GetLongPackageAssetName(BasePackageName) + AssetNameSuffix;
	const FString PackageName = PackagePath + TEXT("/") + AssetName;

	UPackage* Package = CreatePackage(*PackageName);

	// Create the Texture
	UGSplatAsset* GSplatAsset = NewObject<UGSplatAsset>(Package, UGSplatAsset::StaticClass(), FName(*AssetName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	GSplatAsset->AddToRoot();

	{
		GSplatAsset->Splats.SplatCount = Positions.Num();

		GSplatAsset->Splats.Positions = MoveTemp(Positions);
		GSplatAsset->Splats.Scales = MoveTemp(Scales);
		GSplatAsset->Splats.Orientations = MoveTemp(Orientations);
		GSplatAsset->Splats.ZeroOrderHarmonicsCoefficientsAndOpacities = MoveTemp(SHAndOpacities);
		if (Normals.Num())
		{
			GSplatAsset->Splats.bHasNormals = true;
			GSplatAsset->Splats.Normals = MoveTemp(Normals);
		}
		if (MetallicRoughness.Num())
		{
			GSplatAsset->Splats.bHasMetallicRoughness = true;
			GSplatAsset->Splats.MetallicRoughness = MoveTemp(MetallicRoughness);
		}

		GSplatAsset->Splats.RecalculateBounds();
	}

	(void)Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(GSplatAsset);
}
