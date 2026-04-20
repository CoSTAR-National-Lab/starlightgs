#include "GSplatAssetTypeActions.h"
#include "GSplatAsset.h"

UClass* FGSplatAssetTypeActions::GetSupportedClass() const
{
	return UGSplatAsset::StaticClass();
}

FText FGSplatAssetTypeActions::GetName() const
{
   return INVTEXT("Gaussian Splat Data");
}

FColor FGSplatAssetTypeActions::GetTypeColor() const
{
	return FColor::Orange;
}

uint32 FGSplatAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::FX;
}

