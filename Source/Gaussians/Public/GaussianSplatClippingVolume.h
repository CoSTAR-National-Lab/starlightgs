#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"


#include "GaussianSplatClippingVolume.generated.h"


UENUM()
enum class EGaussianSplatClippingVolumeMode : uint8
{
	ClipAllInside = 0 UMETA(DisplayName="Clip Entirely Inside"),
	ClipAnyInside     UMETA(DisplayName="Clip Any Inside"),
	ClipAllOutside    UMETA(DisplayName="Clip Entirely Outside"),
	ClipAnyOutside    UMETA(DisplayName="Clip Any Outside"),
};


UCLASS(hidecategories = (Collision, Brush, Attachment, Physics, Volume, BrushBuilder))
class GAUSSIANS_API AGaussianSplatClippingVolume : public AVolume
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category="Gaussian Splat Clipping Volume")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = "Gaussian Splat Clipping Volume")
	EGaussianSplatClippingVolumeMode Mode = EGaussianSplatClippingVolumeMode::ClipAllInside;

	UPROPERTY(EditAnywhere, Category = "Gaussian Splat Clipping Volume")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category = "Gaussian Splat Clipping Volume")
	int32 Mask = 0;

public:
	AGaussianSplatClippingVolume();

	//~ Begin UObject interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginDestroy() override;
	//~ End UObject interface

};


// Render thread representation of GS clipping volume
struct GAUSSIANS_API FGSClippingVolumeParameters
{
	EGaussianSplatClippingVolumeMode Mode;
	int32 Priority;

	FBox Bounds;

	int32 Mask;

	explicit FGSClippingVolumeParameters(const AGaussianSplatClippingVolume* ClippingVolume);
};
