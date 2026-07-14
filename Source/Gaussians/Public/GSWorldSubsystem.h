

#pragma once

#include "CoreMinimal.h"
#include "GaussianSplatComponent.h"
#include "Subsystems/WorldSubsystem.h"

#include "GSWorldSubsystem.generated.h"
/**
 * 
 */

class AGaussianSplatClippingVolume;

UCLASS()
class GAUSSIANS_API UGSWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	TWeakPtr<class FGSViewExtension> GetViewExtension() const { return GaussianRendererViewExtension; }

	void AddClippingVolume(const AGaussianSplatClippingVolume* ClippingVolume);
	void RemoveClippingVolume(const AGaussianSplatClippingVolume* ClippingVolume);

	// Also removes all nullptr clipping volumes from list (occurs when a volume is removed from the world but not yet destroyed)
	const TArray<const AGaussianSplatClippingVolume*>& GetClippingVolumes();

	// Create a new splat asset by applying clipping volumes in the scene to an existing splat model
	// Don't call every frame as this will stall game thread on render thread
	void ExportClippedSplat(class UGaussianSplatComponent* Component);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TSharedPtr<class FGSViewExtension> GaussianRendererViewExtension;

	UPROPERTY()
	TArray<const AGaussianSplatClippingVolume*> ClippingVolumes;
};
