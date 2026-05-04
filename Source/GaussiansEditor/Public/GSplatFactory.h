// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "GSplatFactory.generated.h"

struct FGaussianSplatData;
class UGSplatImportData;


/**
 * 
 */
UCLASS()
class GAUSSIANSEDITOR_API UGSplatFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UGSplatFactory ();

	//~ Begin UFactory interface
	virtual bool ShouldShowInNewMenu() const override;
	virtual FText GetDisplayName() const override;

	// Create Asset
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	virtual bool CanCreateNew() const override;

	// Import Asset
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, 
		const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual bool DoesSupportClass(UClass* Class) override;
	virtual UClass* ResolveSupportedClass() override;
	//~ End UFactory interface

	//~ Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ End FReimportHandler interface

private:
	UObject* ImportInternal(
		UClass* InClass, 
		UObject* InParent, 
		FName InName, 
		EObjectFlags Flags, 
		const FString& Filename, 
		const TCHAR* Parms, 
		bool& bOutOperationCanceled, 
		bool bIsReimport,
		class UGSplatAsset* InExistingAsset = nullptr);
};

#endif