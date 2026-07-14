

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

	inline const TArray<const AGaussianSplatClippingVolume*>& GetClippingVolumes();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TSharedPtr<class FGSViewExtension> GaussianRendererViewExtension;

	UPROPERTY()
	TArray<const AGaussianSplatClippingVolume*> ClippingVolumes;
};
