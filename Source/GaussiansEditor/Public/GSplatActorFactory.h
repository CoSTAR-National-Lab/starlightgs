// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "GSplatActorFactory.generated.h"

/**
 * 
 */
UCLASS()
class GAUSSIANSEDITOR_API UGSplatActorFactory : public UActorFactory
{
	GENERATED_BODY()

	UGSplatActorFactory();

public:
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;

protected:
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};
