#include "GSViewExtension.h"

#include "Gaussians.h"
#include "GaussianSplatSceneProxy.h"
#include "Rendering/GSGPUStats.h"

#include "RenderGraphUtils.h"
#include "PostProcess/PostProcessInputs.h"
#include "ScenePrivate.h"

#include "GSWorldSubsystem.h"

DEFINE_STAT(STAT_SplatsSubmittedLit)
DEFINE_STAT(STAT_SplatsSubmittedUnlit)
DEFINE_STAT(STAT_SplatsRenderedLit)
DEFINE_STAT(STAT_SplatsRenderedUnlit)

DECLARE_GPU_STAT_NAMED(GaussiansLit, TEXT("Gaussians: Lit"));


static TAutoConsoleVariable<bool> CVarGaussiansDisableViewExtension(
	TEXT("r.Gaussians.DisableViewExtension"),
	false,
	TEXT("Debug tool to disable primary view rendering via view extension."),
	ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<bool> CVarGaussiansForceCullEveryFrame(
	TEXT("r.Gaussians.CullEveryFrame"),
	false,
	TEXT("Forces splat culling (against clipping volumes) to run every frame."),
	ECVF_RenderThreadSafe
);


FGSViewExtension::FGSViewExtension(const FAutoRegister& AutoRegister, UGSWorldSubsystem* Owner)
	: FSceneViewExtensionBase(AutoRegister)
	, OwningSubsystem(Owner)
{
	GPUStats = MakePimpl<FGSGPUStats>();
}

void FGSViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	// Build collection of clipping volumes and send to render thread
	const TArray<const AGaussianSplatClippingVolume*>& ClippingVolumes = OwningSubsystem->GetClippingVolumes();

	TArray<FGSClippingVolumeParameters> TempClippingVolumeParameters;
	TempClippingVolumeParameters.Reserve(ClippingVolumes.Num());

	for (const AGaussianSplatClippingVolume* Volume : ClippingVolumes)
	{
		if (!Volume->bEnabled)
			continue;

		TempClippingVolumeParameters.Emplace(Volume);
	}

	Algo::Sort(TempClippingVolumeParameters, [](const auto& VolA, const auto& VolB){ return VolA.Priority < VolB.Priority; });

	ENQUEUE_RENDER_COMMAND(CopyGaussianSplatClippingVolumes)([this, TempVolumes = MoveTemp(TempClippingVolumeParameters)](FRHICommandList& RHICommandList)
	{
		ClippingVolumeParameters = TempVolumes;
	});
}

void FGSViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	
}

void FGSViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FGSViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	GPUStats->ProcessBeginRender(GraphBuilder);
}

void FGSViewExtension::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
                                                               const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
{
	RDG_EVENT_SCOPE_STAT(GraphBuilder, GaussiansLit, "Gaussians(Lit)");
	RDG_GPU_STAT_SCOPE(GraphBuilder, GaussiansLit);
	SCOPED_NAMED_EVENT(GaussiansLit, FColor::Purple);

	// Run culling for all splats that need it
	{
		bool bForceEnable = CVarGaussiansForceCullEveryFrame.GetValueOnRenderThread();
		TSet<FGaussianSplatSceneProxy*> AllProxies = RelightableProxies.Union(NonRelightableProxies);

		bool bRequiresCullingPass = bForceEnable;
		if (!bForceEnable)
		{
			for (auto Proxy : AllProxies)
			{
				bRequiresCullingPass |= Proxy->RequiresNewCullingPass();
			}
		}

		if (bRequiresCullingPass)
		{
			GSCullingPass(
				GraphBuilder,
				InView,
				AllProxies,
				ClippingVolumeParameters,
				bForceEnable
			);
		}
	}

	// Process relightable proxies
	if (!RelightableProxies.IsEmpty() && Gaussians::IsRelightingEnabled_RenderThread())
	{
		FGSplatGBuffer GSplatGBuffer;
		GSplatGBuffer.Init(GraphBuilder, InView);

		{
			RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Clear GSplatGBuffer");
			GSplatGBuffer.Clear(GraphBuilder);
		}

		GaussianSplattingPass(GraphBuilder, InView, GSplatGBuffer.GetRenderTargetBindings(), RelightableProxies, GPUStats.Get());
		GSCompositionPass(GraphBuilder, InView, GSplatGBuffer);
	}
}

void FGSViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView,
	const FPostProcessingInputs& Inputs)
{
	// Render all splats that don't support relighting
	if (!NonRelightableProxies.IsEmpty() || !Gaussians::IsRelightingEnabled_RenderThread())
	{
		check(InView.bIsViewInfo);
		const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);

		RDG_EVENT_SCOPE(GraphBuilder, "Gaussians: Unlit");
		const FSceneTextureUniformParameters* SceneTextures = Inputs.SceneTextures->GetContents();
		FRDGTextureRef SceneColorTexture = SceneTextures->SceneColorTexture;
		FRDGTextureRef SceneVelocityTexture = ViewInfo.GetSceneTextures().Velocity;
		FRDGTextureRef SceneDepthTexture = SceneTextures->SceneDepthTexture;

		FRenderTargetBindingSlots RenderTargets{};
		RenderTargets[0] = FRenderTargetBinding{ SceneColorTexture, ERenderTargetLoadAction::ELoad };
		RenderTargets[1] = FRenderTargetBinding{ SceneVelocityTexture, ERenderTargetLoadAction::ELoad };
		RenderTargets.DepthStencil = FDepthStencilBinding{
			SceneDepthTexture, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthRead_StencilNop
		};

		if (!Gaussians::IsRelightingEnabled_RenderThread())
		{
			// If relighting is disabled, then render all relightable gaussians with non-relightable gaussians
			TSet<FGaussianSplatSceneProxy*> AllProxies = RelightableProxies.Union(NonRelightableProxies);

			GaussianSplattingPass(GraphBuilder, InView, RenderTargets, AllProxies, GPUStats.Get());
		}
		else
		{
			GaussianSplattingPass(GraphBuilder, InView, RenderTargets, NonRelightableProxies, GPUStats.Get());
		}
	}
}

void FGSViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	GPUStats->ProcessEndRender(GraphBuilder);
}

bool FGSViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	//check if main viewport is valid and if the world of the context match the world of the owning subsystem
	return
		(
			Context.GetWorld() == OwningSubsystem->GetWorld()
		) && 
		!CVarGaussiansDisableViewExtension.GetValueOnGameThread();
}

void FGSViewExtension::RegisterProxy(FGaussianSplatSceneProxy* Proxy)
{
	check(IsInRenderingThread());
	if (Proxy->SupportsRelighting())
	{
		check(!RelightableProxies.Contains(Proxy));
		RelightableProxies.Add(Proxy);
	}
	else
	{
		check(!NonRelightableProxies.Contains(Proxy));
		NonRelightableProxies.Add(Proxy);
	}
}

void FGSViewExtension::UnregisterProxy(FGaussianSplatSceneProxy* Proxy)
{
	check(IsInRenderingThread());
	if (Proxy->SupportsRelighting())
	{
		check(RelightableProxies.Contains(Proxy));
		RelightableProxies.Remove(Proxy);
	}
	else
	{
		check(NonRelightableProxies.Contains(Proxy));
		NonRelightableProxies.Remove(Proxy);
	}
}
