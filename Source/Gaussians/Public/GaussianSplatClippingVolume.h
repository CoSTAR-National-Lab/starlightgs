#pragma once 

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Volume.h"

#include "GaussianSplatClipping.h"


#include "GaussianSplatClippingVolume.generated.h"


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

	// This culling volume will be applied to any Gaussian Splat component whose tags intersect with the MaskTags.
	UPROPERTY(EditAnywhere, Category = "Gaussian Splat Clipping Volume")
	FGameplayTagContainer ClippingTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gaussian Splat Clipping Volume", DisplayName = "Exact Match Only")
	bool bClippingTagsExactMatchOnly = false;

public:
	AGaussianSplatClippingVolume();

	//~ Begin UObject interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject interface

	void PopulateClippingVolumeProxy(FGSClippingVolumeProxy& OutProxy) const;

	// Will force all splats to re-evaluate their culling
	// Call when programatically moving culling volumes
	void MarkClippingStateDirty() const;
};
