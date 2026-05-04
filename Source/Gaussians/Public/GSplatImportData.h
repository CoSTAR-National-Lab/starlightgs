#pragma once

#include "CoreMinimal.h"

#include "GSplatImportData.generated.h"


UENUM()
enum class EGSplatSwizzleAxis : uint8
{
	X = 0,
	Y,
	Z,
	Xn,
	Yn,
	Zn,

	Max
};


/**
 * Cached with the GSplat asset so if it is reimported, the import settings can be recovered
 */
UCLASS()
class UGSplatImportData : public UObject
{
	GENERATED_BODY()

public:

	// Optional property support
	UPROPERTY()
	bool bHasNormals = false;
	UPROPERTY()
	bool bHasHighOrderSH = false;
	UPROPERTY()
	bool bHasMetallicRoughness = false;

	UPROPERTY()
	bool bImportNormals = false;
	UPROPERTY()
	bool bImportHighOrderSH = false;
	UPROPERTY()
	bool bImportMetallicRoughness = false;

	// Transformation on import
	UPROPERTY()
	float ScaleFactor = 100.0f;

	UPROPERTY()
	bool bApplyGammaCorrection = false;

	// Swizzling
	UPROPERTY()
	TArray<EGSplatSwizzleAxis> Swizzle{ EGSplatSwizzleAxis::Z, EGSplatSwizzleAxis::X, EGSplatSwizzleAxis::Yn };
};
