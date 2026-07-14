// Fill out your copyright notice in the Description page of Project Settings.


#include "GSplatActorFactory.h"

#include "GaussianSplatComponent.h"


UGSplatActorFactory::UGSplatActorFactory()
{
	DisplayName = FText::FromString("GSplatActorFactory");
	NewActorClass = AGaussianSplatActor::StaticClass();
    bUseSurfaceOrientation = true;
}

bool UGSplatActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
    // Return true if the dragged asset is your custom asset type
    if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UGSplatAsset::StaticClass()))
    {
        return true;
    }

    return false;
}

void UGSplatActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

    auto Splactor = Cast<AGaussianSplatActor>(NewActor);
    auto Splasset = Cast<UGSplatAsset>(Asset);

    Splactor->GaussianSplatComponent->GaussianSplatAsset = Splasset;
}
