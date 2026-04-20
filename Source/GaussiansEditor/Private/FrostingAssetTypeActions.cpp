#include "FrostingAssetTypeActions.h"
#include "Frosting/FrostingAsset.h"


UClass* FFrostingAssetTypeActions::GetSupportedClass() const
{
	return UFrostingDataAsset::StaticClass();
}

FText FFrostingAssetTypeActions::GetName() const
{
	return INVTEXT("Frosting Data");
}

FColor FFrostingAssetTypeActions::GetTypeColor() const
{
	FColor DarkerOrange(FColor::Orange); DarkerOrange.R += 5, DarkerOrange.B += 3; return DarkerOrange;
}

uint32 FFrostingAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::FX;
}

bool FFrostingAssetTypeActions::IsImportedAsset() const
{
	return true;
}
