

#pragma once

#include "CoreMinimal.h"
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

private:
	TSharedPtr<class FGSViewExtension> GaussianRendererViewExtension;

	UPROPERTY()
	TArray<const AGaussianSplatClippingVolume*> ClippingVolumes;
};
