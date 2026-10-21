#include "GSplatGBuffer.h"

#include "RenderGraphBuilder.h"
#include "ScenePrivate.h"


FGSplatGBuffer::FGSplatGBuffer()
	: BaseColorTexture(nullptr)
	, NormalTexture(nullptr)
	, DepthMetallicRoughnessTexture(nullptr)
	, VelocityTexture(nullptr)
{
}

void FGSplatGBuffer::Init(FRDGBuilder& GraphBuilder, const FSceneView& InView)
{
	check(IsInRenderingThread());
	check(InView.bIsViewInfo);

	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);
	const FIntPoint Extent = ViewInfo.ViewRect.Size();

	ETextureCreateFlags Flags = TexCreate_ShaderResource | TexCreate_RenderTargetable;
	FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(Extent, PF_FloatRGBA, FClearValueBinding::Transparent, Flags);
	BaseColorTexture			  = GraphBuilder.CreateTexture(Desc, TEXT("GSplatGBuffer.BaseColor"));
	NormalTexture				  = GraphBuilder.CreateTexture(Desc, TEXT("GSplatGBuffer.Normal"));
	DepthMetallicRoughnessTexture = GraphBuilder.CreateTexture(Desc, TEXT("GSplatGBuffer.DepthMetallicRoughness"));
	VelocityTexture				  = GraphBuilder.CreateTexture(Desc, TEXT("GSplatGBuffer.Velocity"));
}

void FGSplatGBuffer::Clear(FRDGBuilder& GraphBuilder) const
{
	AddClearRenderTargetPass(GraphBuilder, BaseColorTexture);
	AddClearRenderTargetPass(GraphBuilder, NormalTexture);
	AddClearRenderTargetPass(GraphBuilder, DepthMetallicRoughnessTexture);
	AddClearRenderTargetPass(GraphBuilder, VelocityTexture);
}

FRenderTargetBindingSlots FGSplatGBuffer::GetRenderTargetBindings(bool bClear, FRDGTextureRef DepthBuffer) const
{
	check(IsInRenderingThread());

	ERenderTargetLoadAction Action = bClear ? ERenderTargetLoadAction::EClear : ERenderTargetLoadAction::ELoad;

	FRenderTargetBindingSlots BindingSlots{ };
	BindingSlots[0] = FRenderTargetBinding(BaseColorTexture, Action);
	BindingSlots[1] = FRenderTargetBinding(NormalTexture, Action);
	BindingSlots[2] = FRenderTargetBinding(DepthMetallicRoughnessTexture, Action);
	BindingSlots[3] = FRenderTargetBinding(VelocityTexture, Action);

	if (DepthBuffer)
	{
		BindingSlots.DepthStencil = FDepthStencilBinding(
			DepthBuffer, 
			ERenderTargetLoadAction::ELoad,
			ERenderTargetLoadAction::ELoad,
			FExclusiveDepthStencil::DepthRead
		);
	}

	return BindingSlots;
}

FGSplatGBufferParameters FGSplatGBuffer::GetShaderParameters(FRDGBuilder& GraphBuilder) const
{
	check(IsInRenderingThread());

	FGSplatGBufferParameters Parameters;

	Parameters.GSplatBaseColor		        = GraphBuilder.CreateSRV(BaseColorTexture);
	Parameters.GSplatNormal			        = GraphBuilder.CreateSRV(NormalTexture);
	Parameters.GSplatDepthMetallicRoughness = GraphBuilder.CreateSRV(DepthMetallicRoughnessTexture);
	Parameters.GSplatVelocity				= GraphBuilder.CreateSRV(VelocityTexture);

	return Parameters;
}
