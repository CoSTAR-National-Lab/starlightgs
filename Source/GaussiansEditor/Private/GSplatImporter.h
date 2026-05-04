#pragma once

class UGSplatImportData;
struct FGaussianSplatData;


namespace happly
{
	class PLYData;
}


namespace GSplatImporter
{
	// Will allocate ply data - please delete yourself later
	bool ParsePly(const FString& Filename, UGSplatImportData& ImportData, happly::PLYData** OutPlyData);
	bool ImportPly(const UGSplatImportData& ImportData, happly::PLYData* PlyData, FGaussianSplatData& Splats);
};
