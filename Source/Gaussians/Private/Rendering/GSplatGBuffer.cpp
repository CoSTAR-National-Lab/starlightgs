#include "GSplatGBuffer.h"

#include "RenderGraphBuilder.h"
#include "ScenePrivate.h"


FGSplatGBuffer::FGSplatGBuffer()
	: BaseColorTexture(nullptr)
	, NormalTexture(nullptr)
	, MetallicRoughnessTexture(nullptr)
	, DepthTexture(nullptr)
{
}

void FGSplatGBuffer::Init(FRDGBuilder& GraphBuilder, const FSceneView& InView)
{
	check(IsInRenderingThread());
	check(InView.bIsViewInfo);

	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);
	const FIntPoint Extent = ViewInfo.ViewRect.Size();

	ETextureCreateFlags Flags = TexCreate_ShaderResource | TexCreate_RenderTargetable;
	ETextureCreateFlags DepthFlags = TexCreate_ShaderResource | TexCreate_DepthStencilTargetable; // splats dont use Depth stencil at all
	BaseColorTexture		 = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_FloatRGBA, FClearValueBinding::Black, Flags), TEXT("GSplatGBuffer.BaseColor"));
	NormalTexture			 = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_FloatRGB, FClearValueBinding::Black, Flags), TEXT("GSplatGBuffer.Normal"));
	MetallicRoughnessTexture = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_R8G8, FClearValueBinding::Black, Flags), TEXT("GSplatGBuffer.MetallicRoughness"));
	DepthTexture			 = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(Extent, PF_R32_FLOAT, FClearValueBinding(FLinearColor((float)ERHIZBuffer::FarPlane, (float)ERHIZBuffer::FarPlane, (float)ERHIZBuffer::FarPlane, (float)ERHIZBuffer::FarPlane)), Flags), TEXT("GSplatGBuffer.Depth"));
}

void FGSplatGBuffer::Clear(FRDGBuilder& GraphBuilder) const
{
	AddClearRenderTargetPass(GraphBuilder, BaseColorTexture);
	AddClearRenderTargetPass(GraphBuilder, NormalTexture);
	AddClearRenderTargetPass(GraphBuilder, MetallicRoughnessTexture);
	AddClearRenderTargetPass(GraphBuilder, DepthTexture);
}

FRenderTargetBindingSlots FGSplatGBuffer::GetRenderTargetBindings(bool bClear) const
{
	check(IsInRenderingThread());

	ERenderTargetLoadAction Action = bClear ? ERenderTargetLoadAction::ELoad : ERenderTargetLoadAction::ELoad;

	FRenderTargetBindingSlots BindingSlots{ };
	BindingSlots[0] = FRenderTargetBinding(BaseColorTexture, Action);
	BindingSlots[1] = FRenderTargetBinding(NormalTexture, Action);
	BindingSlots[2] = FRenderTargetBinding(MetallicRoughnessTexture, Action);
	BindingSlots[3] = FRenderTargetBinding(DepthTexture, Action);
	//BindingSlots.DepthStencil = FDepthStencilBinding(DepthTexture, Action, 
	//												 ERenderTargetLoadAction::ENoAction, FExclusiveDepthStencil::DepthWrite_StencilNop);

	return BindingSlots;
}

FGSplatGBufferParameters FGSplatGBuffer::GetShaderParameters(FRDGBuilder& GraphBuilder) const
{
	check(IsInRenderingThread());

	FGSplatGBufferParameters Parameters;

	Parameters.GSplatBaseColor		 = GraphBuilder.CreateSRV(BaseColorTexture);
	Parameters.GSplatNormal			 = GraphBuilder.CreateSRV(NormalTexture);
	Parameters.GSplatMetallicRoughness = GraphBuilder.CreateSRV(MetallicRoughnessTexture);
	Parameters.GSplatDepth			 = GraphBuilder.CreateSRV(DepthTexture);

	return Parameters;
}
