#include "GSplatAsset.h"

#include "GaussianSplatResource.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#endif


FGaussianSplatResourceWrapper::FGaussianSplatResourceWrapper(FKey&&, TPimplPtr<FGaussianSplatResource>&& InResource)
	: Resource(MoveTemp(InResource))
{
	check(Resource);

	if (IsInRenderingThread() || IsInParallelRenderingThread())
	{
		Resource->InitResource(FRHICommandListImmediate::Get());
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(CreateGaussianSplatResources)(
			[this](FRHICommandListImmediate& RHICommandList)
			{
				Resource->InitResource(RHICommandList);
			});
	}
}

FGaussianSplatResourceWrapper::~FGaussianSplatResourceWrapper()
{
	if (IsInRenderingThread() || IsInParallelRenderingThread())
	{
		Resource->ReleaseResource();
	}
	else
	{
		ENQUEUE_RENDER_COMMAND(DestroyGaussianSplatResources)(
			[this](FRHICommandListImmediate& RHICommandList)
			{
				Resource->ReleaseResource();
			});
	}
}

void UGSplatAsset::BeginDestroy()
{
	WeakWrapper.Reset();

	Super::BeginDestroy();
}

void UGSplatAsset::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
}

#if WITH_EDITOR

void UGSplatAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// Check if any properties have changed that would trigger re-creating GPU resources
	bool bSplatsModified = false;
	bSplatsModified |= PropertyName == GET_MEMBER_NAME_CHECKED(FGaussianSplatData, bUseNormals);
	bSplatsModified |= PropertyName == GET_MEMBER_NAME_CHECKED(FGaussianSplatData, bUseHighOrderSH);
	bSplatsModified |= PropertyName == GET_MEMBER_NAME_CHECKED(FGaussianSplatData, bUseMetallicRoughness);

	if (bSplatsModified)
	{
		RecreateResource();
	}
}

void UGSplatAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);
#if WITH_EDITORONLY_DATA
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}
#endif
}

#endif

void UGSplatAsset::RecalculateBounds()
{
	Splats.RecalculateBounds();
	(void)MarkPackageDirty();
}


TSharedPtr<FGaussianSplatResourceWrapper> UGSplatAsset::GetOrCreateResource()
{
	check(IsInGameThread());

	if (auto Wrapper = WeakWrapper.Pin())
	{
		return Wrapper;
	}

	// Don't create a resource wrapper if there are no splats in this asset.
	// Proxies will not be registered with the view extension in this case.
	// Proxies will be re-created if the asset is modified (and then registered with view extension)
	if (Splats.SplatCount == 0)
	{
		return nullptr;
	}

	// Resources have not been created yet
	auto GPUSplats = MakePimpl<FGaussianSplatResource>(Splats);

	TSharedPtr<FGaussianSplatResourceWrapper> Wrapper = MakeShared<FGaussianSplatResourceWrapper>(FGaussianSplatResourceWrapper::FKey{}, MoveTemp(GPUSplats));
	WeakWrapper = Wrapper;

	return Wrapper;
}

void UGSplatAsset::DetachResource()
{
	WeakWrapper.Reset();
}

void UGSplatAsset::RecreateResource() const
{
	check(IsInGameThread());

	if (auto Wrapper = WeakWrapper.Pin())
	{
		FGaussianSplatData SplatsTemp = Splats;

		ENQUEUE_RENDER_COMMAND(RecreateGaussianSplatResources)(
			[Wrapper, SplatsTemp](FRHICommandListImmediate& RHICommandList)
			{
				// Will release and re-init RHI resources
				Wrapper->GetResource()->UpdateSplats(RHICommandList, SplatsTemp);
			});

		// Splat data has changed - components should re-create their render state
		OnAssetUpdate.Broadcast();
	}
}
