

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GSWorldSubsystem.generated.h"
/**
 * 
 */

UCLASS()
class GAUSSIANS_API UGSWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UTexture2DArray* Output;

	TWeakPtr<class FGSViewExtension> GetViewExtension() const { return GaussianRendererViewExtension; }

private:
	TSharedPtr<class FGSViewExtension> GaussianRendererViewExtension;

};