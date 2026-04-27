#pragma once

#include "ShaderParameterMacros.h"


BEGIN_SHADER_PARAMETER_STRUCT(FGSplatGBufferParameters, )
	SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, GSplatBaseColor)
	SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float3>, GSplatNormal)
	SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float2>, GSplatMetallicRoughness)
	SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>,  GSplatDepth)
END_SHADER_PARAMETER_STRUCT()


// Collection of transient textures that make up the GSplat GBuffer
// Which is then composited with the scene renderer's GBuffer to enable relighting
class FGSplatGBuffer
{
public:
	FGSplatGBuffer();

	// Allocates textures
	void Init(FRDGBuilder& GraphBuilder, const FSceneView& InView);

	// Clears all render targets in the GSplatGBuffer to their clear value
	void Clear(FRDGBuilder& GraphBuilder) const;

	// Use this to bind GSplatGBuffer as render targets. Also binds depth-stencil view
	// If bClear = true, render targets will be cleared on load
	FRenderTargetBindingSlots GetRenderTargetBindings(bool bClear = false) const;

	// Use this to bind GSplatGBuffer as collection of shader parameters for reading
	FGSplatGBufferParameters GetShaderParameters(FRDGBuilder& GraphBuilder) const;

	FRDGTextureRef GetBaseColorTexture() const { return BaseColorTexture; }

private:
	FRDGTextureRef BaseColorTexture;
	FRDGTextureRef NormalTexture;
	FRDGTextureRef MetallicRoughnessTexture;
	FRDGTextureRef DepthTexture;
};
