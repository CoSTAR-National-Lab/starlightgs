#include "FrostingFactory.h"

#include "GaussiansEditorModule.h"
#include "Frosting/FrostingAsset.h"
#include "EditorFramework/AssetImportData.h"

#include <happly.h>


UFrostingFactory::UFrostingFactory()
{
	SupportedClass = UFrostingDataAsset::StaticClass();
	bCreateNew = true;

	Formats.Add(TEXT("frosting;FrostingImport"));

	//bText = false;
	bEditorImport = true;

}

bool UFrostingFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if (Extension == TEXT("frosting"))
	{
		return true;
	}

	return false;
}

UObject* UFrostingFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	auto object = NewObject<UFrostingDataAsset>(InParent, Class, Name, Flags);

	if (!GetCurrentFilename().IsEmpty())
	{
		object->AssetImportData = NewObject<UAssetImportData>(object);
		object->AssetImportData->Update(GetCurrentFilename());

		std::string file = std::string(TCHAR_TO_UTF8(*GetCurrentFilename()));
		auto ply = happly::PLYData(file, true);

		auto& vertices = ply.getElement("vertex");

		object->Frosting.Mesh.Vertices.Init(FVector(0, 0, 0), vertices.count);
		object->Frosting.Mesh.InnerOuterPrismDistance.Init(FVector2D(0, 0), vertices.count);

		std::vector<double> x = vertices.getProperty<double>("x");
		std::vector<double> y = vertices.getProperty<double>("y");
		std::vector<double> z = vertices.getProperty<double>("z");

		std::vector<double> I = vertices.getProperty<double>("inner_dist");
		std::vector<double> O = vertices.getProperty<double>("outer_dist");

		for (int i = 0; i < vertices.count; i++)
		{
			object->Frosting.Mesh.Vertices[i].X = z[i] * 100.f;
			object->Frosting.Mesh.Vertices[i].Y = x[i] * 100.f;
			object->Frosting.Mesh.Vertices[i].Z = -y[i] * 100.f;

			object->Frosting.Mesh.InnerOuterPrismDistance[i].X = I[i] * 100.f;
			object->Frosting.Mesh.InnerOuterPrismDistance[i].Y = O[i] * 100.f;
		}

		auto& indices = ply.getElement("face");
		object->Frosting.Mesh.Indices.Init(0, indices.count * 3);

		std::vector<std::vector<unsigned int>> Indices = indices.getListProperty<unsigned int>("vertex_indices");

		for (int i = 0; i < indices.count; i++)
		{
			object->Frosting.Mesh.Indices[i * 3] = Indices[i][0];
			object->Frosting.Mesh.Indices[i * 3 + 1] = Indices[i][1];
			object->Frosting.Mesh.Indices[i * 3 + 2] = Indices[i][2];
		}

		auto& gaussian_indices = ply.getElement("gaussian_index");
		auto& gaussian_bary_coords = ply.getElement("gaussian_bary_coords");
		auto& gaussian_scales = ply.getElement("gaussian_scales");
		auto& gaussian_quaternions = ply.getElement("gaussian_quaternions");
		auto& gaussian_opacity = ply.getElement("gaussian_opacity");
		auto& gaussian_sh_dc = ply.getElement("gaussian_sh_dc");
		auto& gaussian_sh_rest = ply.getElement("gaussian_sh_rest");

		object->Frosting.Splats.Resize(gaussian_indices.count);

		std::vector<int> faceidx = gaussian_indices.getProperty<int>("i");

		std::vector<std::vector<double>> bary = gaussian_bary_coords.getListProperty<double>("bary_coords");
		std::vector<std::vector<float>> scales = gaussian_scales.getListProperty<float>("scales");
		std::vector<std::vector<double>> quats = gaussian_quaternions.getListProperty<double>("quaternions");
		std::vector<double> opacities = gaussian_opacity.getProperty<double>("opacity");
		std::vector<std::vector<float>> sh_dc = gaussian_sh_dc.getListProperty<float>("sh_dc_coordinates");
		std::vector<std::vector<double>> sh_rest = gaussian_sh_rest.getListProperty<double>("sh_rest_coordinates");

		//apply exponent to scales
		for (auto& s : scales)
		{
			s[0] = FMath::Exp(s[0]);
			s[1] = FMath::Exp(s[1]);
			s[2] = FMath::Exp(s[2]);
		}

		for (auto& o : opacities)
		{
			o = 1.f / (1.0f + FMath::Exp(-o));
		}

		//apply 

		for (int i = 0; i < gaussian_indices.count; i++)
		{
			object->Frosting.Splats.FaceIndices[i] = faceidx[i];

			auto softmax = [](const std::vector<double>& input)
				{
					// 1. Find the maximum element for numerical stability
					float max_val = *std::ranges::max_element(input);

					std::vector<double> output;
					output.reserve(input.size());

					// 2. Compute exponents and the sum of exponents
					double sum = 0.0f;
					for (double x : input) {
						double e = std::exp(x - max_val);
						output.push_back(e);
						sum += e;
					}

					// 3. Normalize the vector
					for (double& x : output) {
						x /= sum;
					}

					return output;
				};

			auto bary_softmaxed = softmax(bary[i]);

			object->Frosting.Splats.BarryCoords[i].a0 = bary_softmaxed[0];
			object->Frosting.Splats.BarryCoords[i].a1 = bary_softmaxed[1];
			object->Frosting.Splats.BarryCoords[i].a2 = bary_softmaxed[2];
			object->Frosting.Splats.BarryCoords[i].a3 = bary_softmaxed[3];
			object->Frosting.Splats.BarryCoords[i].a4 = bary_softmaxed[4];
			object->Frosting.Splats.BarryCoords[i].a5 = bary_softmaxed[5];


			object->Frosting.Splats.Scales[i].X = scales[i][2]; //Z
			object->Frosting.Splats.Scales[i].Y = scales[i][0]; //X
			object->Frosting.Splats.Scales[i].Z = scales[i][1]; //Y

			object->Frosting.Splats.Orientations[i].W = quats[i][0]; //W
			object->Frosting.Splats.Orientations[i].X = quats[i][3]; //Z
			object->Frosting.Splats.Orientations[i].Y = quats[i][1]; //X
			object->Frosting.Splats.Orientations[i].Z = -quats[i][2]; //-Y

			object->Frosting.Splats.Orientations[i].Normalize();

			object->Frosting.Splats.SH0AndOpacities[i].X = sh_dc[i][0];
			object->Frosting.Splats.SH0AndOpacities[i].Y = sh_dc[i][1];
			object->Frosting.Splats.SH0AndOpacities[i].Z = sh_dc[i][2];
			object->Frosting.Splats.SH0AndOpacities[i].W = opacities[i];
		}
	}

	object->UpdateAssetCounters();

	return object;
}

bool UFrostingFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UFrostingDataAsset* pGS = Cast<UFrostingDataAsset>(Obj);
	if (pGS && pGS->AssetImportData)
	{
		pGS->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void UFrostingFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UFrostingDataAsset* pGS = Cast<UFrostingDataAsset>(Obj);
	if (pGS && ensure(NewReimportPaths.Num() == 1))
	{
		if (pGS->AssetImportData)
		{
			pGS->AssetImportData = NewObject<UAssetImportData>(pGS);
			pGS->AssetImportData->Update(NewReimportPaths[0]);
		}
		else
			pGS->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UFrostingFactory::Reimport(UObject* Obj)
{
	TGuardValue<bool> IsDoingAReimportGuard(bIsDoingAReimport, true);

	if (!Obj || !Obj->IsA(UFrostingDataAsset::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UFrostingDataAsset* pGS = Cast<UFrostingDataAsset>(Obj);

	const FString ResolvedSourceFilePath = pGS->AssetImportData->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		return EReimportResult::Failed;
	}

	UE_LOG(LogGSplatEditor, Log, TEXT("Performing reimport of [%s]"), *ResolvedSourceFilePath);

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		UE_LOG(LogGSplatEditor, Warning, TEXT("-- cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	bool OutCanceled = false;

	if (ImportObject(pGS->GetClass(), pGS->GetOuter(), *pGS->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogGSplatEditor, Log, TEXT("-- imported successfully"));

		pGS->AssetImportData->Update(ResolvedSourceFilePath);

		return EReimportResult::Succeeded;
	};

	return EReimportResult::Failed;
}
