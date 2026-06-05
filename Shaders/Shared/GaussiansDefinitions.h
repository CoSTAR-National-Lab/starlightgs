#pragma once


#ifdef __cplusplus
#include "HLSLTypeAliases.h"

namespace UE::HLSL
{
#endif

struct FGaussians_ClippingVolume
{
	float3 Min;
	float3 Max;
	uint Mode;
};

#define GAUSSIANS_CLIPPING_MODE_CLIP_INSIDE_ALL  0
#define GAUSSIANS_CLIPPING_MODE_CLIP_INSIDE_ANY  1
#define GAUSSIANS_CLIPPING_MODE_CLIP_OUTSIDE_ALL 2
#define GAUSSIANS_CLIPPING_MODE_CLIP_OUTSIDE_ANY 3

#ifdef __cplusplus
}

using FGaussians_ClippingVolume = UE::HLSL::FGaussians_ClippingVolume;

#endif
