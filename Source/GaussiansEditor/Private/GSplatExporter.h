#pragma once

#include "CoreMinimal.h"
#include "GSplatImportData.h"

class UGSplatAsset;
struct FScopedSlowTask;


/**
 * Coordinate space the exported PLY file should be written in.
 */
enum class EGSplatExportCoordinateSpace : uint8
{
	// Invert the import swizzle and scale factor so the file matches the space of the
	// original PLY (requires the asset to have cached import data).
	OriginalCapture,

	// Export the data exactly as stored in the asset (Unreal engine space).
	UnrealEngine
};


struct FGSplatExportOptions
{
	EGSplatExportCoordinateSpace CoordinateSpace = EGSplatExportCoordinateSpace::OriginalCapture;
	bool bIncludeHighOrderSH = true;
	bool bIncludeNormals = true;
	bool bIncludeMetallicRoughness = true;
};


class FGaussianSplatDataExtension
{
public:
	static void RegisterMenus();
	static void ExportPLY(const struct FToolMenuContext& MenuContext);
};


namespace GSplatExporter
{
	bool ExportPly(const UGSplatAsset& InAsset, const FGSplatExportOptions& InOptions, const FString& InFilename, FScopedSlowTask* InProgress = nullptr);
};
