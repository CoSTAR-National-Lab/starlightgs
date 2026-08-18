#pragma once 

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GaussianSplatClipping.generated.h"


UENUM()
enum class EGaussianSplatClippingVolumeMode : uint8
{
	ClipAllInside = 0 UMETA(DisplayName = "Clip Entirely Inside"),
	ClipAnyInside     UMETA(DisplayName = "Clip Any Inside"),
	ClipAllOutside    UMETA(DisplayName = "Clip Entirely Outside"),
	ClipAnyOutside    UMETA(DisplayName = "Clip Any Outside"),
};


// Render thread representation of GS clipping volume
struct GAUSSIANSRENDERER_API FGSClippingVolumeProxy
{
	EGaussianSplatClippingVolumeMode Mode = EGaussianSplatClippingVolumeMode::ClipAllInside;
	int32 Priority = 0;

	FBox Bounds{ };

	FGameplayTagContainer Tags{ };
	bool bExactMatchOnly = false;
};


class GAUSSIANSRENDERER_API IGSClippingVolumeContainerInterface
{
public:
	virtual ~IGSClippingVolumeContainerInterface() = default;

	virtual TArray<FGSClippingVolumeProxy> BuildClippingVolumeProxies() = 0;
};
