#pragma once

#include "CoreMinimal.h"

class AGaussianSplatClippingVolume;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnClippingVolumeNeedsBrushRebuild, AGaussianSplatClippingVolume*);


class GAUSSIANS_API FGSClippingVolumeRebuildService
{
public:
	static FOnClippingVolumeNeedsBrushRebuild& GetOnNeedsRebuild()
	{
		static FOnClippingVolumeNeedsBrushRebuild OnClippingVolumeNeedsBrushRebuild;
		return OnClippingVolumeNeedsBrushRebuild;
	}

	static void RequestRebuild(AGaussianSplatClippingVolume* ClippingVolume)
	{
		GetOnNeedsRebuild().Broadcast(ClippingVolume);
	}
};
