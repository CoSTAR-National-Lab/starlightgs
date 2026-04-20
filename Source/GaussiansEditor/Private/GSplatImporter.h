#pragma once


struct FGaussianSplatData;

namespace happly
{
	class PLYData;
}

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


class FGSplatImportData
{
public:

	static bool ParsePly(const FString& Filename, FGSplatImportData& ImportData);
	static bool ImportPly(const FGSplatImportData& ImportData, FGaussianSplatData& Splats);

public:
	~FGSplatImportData();

	// Optional property support
	bool bHasNormals = false;
	bool bHasHighOrderSH = false;
	bool bHasMetallicRoughness = false;

	bool bImportNormals = false;
	bool bImportHighOrderSH = false;
	bool bImportMetallicRoughness = false;

	// Transformation on import
	float ScaleFactor = 100.0f;

	// Swizzling
	TStaticArray<EGSplatSwizzleAxis, 3> Swizzle{ EGSplatSwizzleAxis::Z, EGSplatSwizzleAxis::X, EGSplatSwizzleAxis::Yn };

private:
	happly::PLYData* PlyData = nullptr;
};
