// Fill out your copyright notice in the Description page of Project Settings.


#include "GSplatCommponentAssetBroker.h"

#include "GSplatAsset.h"
#include "GaussianSplatComponent.h"


UClass* FGSplatCommponentAssetBroker::GetSupportedAssetClass()
{
	return UGSplatAsset::StaticClass();
}

bool FGSplatCommponentAssetBroker::AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset)
{
    if (UGaussianSplatComponent* Comp = Cast<UGaussianSplatComponent>(InComponent))
    {
        UGSplatAsset* Asset = Cast<UGSplatAsset>(InAsset);
        if (Asset)
        {
            Comp->GaussianSplatAsset = Asset;
            return true;
        }
    }
    return false;

}

UObject* FGSplatCommponentAssetBroker::GetAssetFromComponent(UActorComponent* InComponent)
{
    if (UGaussianSplatComponent* MyComp = Cast<UGaussianSplatComponent>(InComponent))
    {
        return MyComp->GaussianSplatAsset;
    }
    return nullptr;
}
