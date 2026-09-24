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

UENUM()
enum class EGaussianSplatClippingVolumeShape : uint8
{
	Box = 0,
	Sphere
};


// Render thread representation of GS clipping volume
struct GAUSSIANSRENDERER_API FGSClippingVolumeProxy
{
	EGaussianSplatClippingVolumeMode Mode = EGaussianSplatClippingVolumeMode::ClipAllInside;
	EGaussianSplatClippingVolumeShape Shape = EGaussianSplatClippingVolumeShape::Box;
	int32 Priority = 0;

	// OBB of clipping volume
	FOrientedBox OrientedBounds{ };

	FGameplayTagContainer Tags{ };
	bool bExactMatchOnly = false;
};


class GAUSSIANSRENDERER_API IGSClippingVolumeContainerInterface
{
public:
	virtual ~IGSClippingVolumeContainerInterface() = default;

	virtual TArray<FGSClippingVolumeProxy> BuildClippingVolumeProxies() = 0;

	// If true, clipping will be re-evaluated on all splats
	// For use in cases where a clipping volume has been added or removed
	virtual bool GetForceClippingEvaluation() const = 0;

	virtual void MarkClippingDirtyFlags() = 0;
	virtual void ClearClippingDirtyFlags() = 0;
};
