// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentAssetBroker.h"

class GAUSSIANSEDITOR_API FGSplatCommponentAssetBroker : public IComponentAssetBroker
{
public:
	virtual UClass* GetSupportedAssetClass() override;
	virtual bool AssignAssetToComponent(UActorComponent * InComponent, UObject * InAsset) override;
	virtual UObject* GetAssetFromComponent(UActorComponent * InComponent) override;
};
