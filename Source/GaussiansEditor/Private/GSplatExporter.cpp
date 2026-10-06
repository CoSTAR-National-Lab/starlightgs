#include "GSplatExporter.h"

#include "GSplatAsset.h"
#include "GaussianData.h"
#include "GaussiansEditorModule.h"

#include "Misc/FeedbackContext.h"

#include "ContentBrowserMenuContexts.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Notifications/SNotificationList.h"

#include <happly.h>

#include "GSplatExportWindow.h"

#define LOCTEXT_NAMESPACE "GSplatExporter"

namespace
{
	// 0.28209479177387814 = sqrt(1 / (4*pi)) - SH DC -> linear RGB scale, matching GSplatImporter
	constexpr float SHDCColorScale = 0.28209479177387814f;
	constexpr float GammaExponent = 2.2f;

	EGSplatSwizzleAxis GetSwizzleAxis(EGSplatSwizzleAxis InSignedAxis)
	{
		switch (InSignedAxis)
		{
		case EGSplatSwizzleAxis::X:
		case EGSplatSwizzleAxis::Xn: return EGSplatSwizzleAxis::X;
		case EGSplatSwizzleAxis::Y:
		case EGSplatSwizzleAxis::Yn: return EGSplatSwizzleAxis::Y;
		case EGSplatSwizzleAxis::Z:
		case EGSplatSwizzleAxis::Zn: return EGSplatSwizzleAxis::Z;
		default:
			checkNoEntry();
			return EGSplatSwizzleAxis::Max;
		}
	}

	bool IsSignedSwizzleAxis(EGSplatSwizzleAxis InAxis)
	{
		switch (InAxis)
		{
		case EGSplatSwizzleAxis::Xn:
		case EGSplatSwizzleAxis::Yn:
		case EGSplatSwizzleAxis::Zn: return true;
		default: return false;
		}
	}

	FVector3f UnitAxisForSwizzle(EGSplatSwizzleAxis InAxis)
	{
		const float Sign = IsSignedSwizzleAxis(InAxis) ? -1.0f : 1.0f;
		switch (GetSwizzleAxis(InAxis))
		{
		case EGSplatSwizzleAxis::X: return FVector3f{ Sign, 0.0f, 0.0f };
		case EGSplatSwizzleAxis::Y: return FVector3f{ 0.0f, Sign, 0.0f };
		case EGSplatSwizzleAxis::Z: return FVector3f{ 0.0f, 0.0f, Sign };
		default:
			checkNoEntry();
			return FVector3f::ZeroVector;
		}
	}

	/**
	 * Inverse of GSplatImporter's SwizzleVector/SwizzleComponent.
	 * Import maps: UnrealAxis[i] = sign(i) * SourceAxis[Swizzle[i]];
	 * this inverts it: SourceAxis[Swizzle[i]] = sign(i) * UnrealAxis[i].
	 */
	FVector3f InverseSwizzleVector(const FVector3f& InV, const TArray<EGSplatSwizzleAxis>& InSwizzle)
	{
		check(InSwizzle.Num() == 3);

		FVector3f Out = FVector3f::ZeroVector;
		for (int32 i = 0; i < 3; i++)
		{
			const int32 SrcAxis = static_cast<int32>(GetSwizzleAxis(InSwizzle[i]));
			const float Sign = IsSignedSwizzleAxis(InSwizzle[i]) ? -1.0f : 1.0f;
			Out.Component(SrcAxis) = Sign * InV.Component(i);
		}
		return Out;
	}

	/**
	 * Inverse of GSplatImporter's SwizzleQuaternion.
	 * The import transform maps the quaternion vector part through the same signed
	 * permutation and flips the scalar part when the mapping is right-handed - except
	 * for an identity swizzle, where the file is already in Unreal engine space and the
	 * quaternion passes through unchanged - so the inverse is: vector part through the
	 * inverse permutation, scalar part flipped under the same condition.
	 */
	FVector4f InverseSwizzleQuaternion(const FQuat4f& InQ, const TArray<EGSplatSwizzleAxis>& InSwizzle)
	{
		check(InSwizzle.Num() == 3);

		const FVector3f XAxis = UnitAxisForSwizzle(InSwizzle[0]);
		const FVector3f YAxis = UnitAxisForSwizzle(InSwizzle[1]);
		const FVector3f ZAxis = UnitAxisForSwizzle(InSwizzle[2]);
		const bool bRightHanded = (FVector3f::DotProduct(FVector3f::CrossProduct(XAxis, YAxis), ZAxis) > 0.0f);

		const FVector3f SrcV = InverseSwizzleVector(FVector3f(InQ.X, InQ.Y, InQ.Z), InSwizzle);
		return FVector4f{ SrcV, bRightHanded ? -InQ.W : InQ.W };
	}

	// Standard 3DGS convention: opacity is stored pre-sigmoid (logit of alpha)
	float LogitOpacity(float InAlpha)
	{
		const float Clamped = FMath::Clamp(InAlpha, 1e-6f, 1.0f - 1e-6f);
		return FMath::Loge(Clamped / (1.0f - Clamped));
	}

	// Standard 3DGS convention: scales are stored in log space
	float LogScale(float InScale)
	{
		return FMath::Loge(FMath::Max(InScale, 1e-8f));
	}

	// Linear RGB -> zero-order SH DC coefficient (inverse of the importer's SH -> RGB conversion)
	float ColorToSHDC(float InColor)
	{
		return (InColor - 0.5f) / SHDCColorScale;
	}
}


bool GSplatExporter::ExportPly(const UGSplatAsset& InAsset, const FGSplatExportOptions& InOptions, const FString& InFilename, FScopedSlowTask* InProgress)
{
	const FGaussianSplatData& Splats = InAsset.Splats;
	const uint32 SplatCount = Splats.SplatCount;

	if (SplatCount == 0)
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Export failed: %s contains no splats."), *InAsset.GetName());
		return false;
	}

	if (Splats.Positions.Num() != SplatCount
		|| Splats.Orientations.Num() != SplatCount
		|| Splats.Scales.Num() != SplatCount
		|| Splats.ZeroOrderHarmonicsCoefficientsAndOpacities.Num() != SplatCount)
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Export failed: %s has inconsistent splat data (expected %u splats)."), *InAsset.GetName(), SplatCount);
		return false;
	}

	const UGSplatImportData* ImportData = InAsset.GSplatImportData;

	// --- Resolve coordinate space ---
	// "Original capture space" requires the cached import settings to invert the
	// swizzle and scale factor applied on import.
	bool bUseOriginalSpace = (InOptions.CoordinateSpace == EGSplatExportCoordinateSpace::OriginalCapture);
	const TArray<EGSplatSwizzleAxis>* Swizzle = nullptr;
	float ScaleFactor = 1.0f;
	bool bInvertGamma = false;

	if (bUseOriginalSpace)
	{
		if (!ImportData || ImportData->Swizzle.Num() != 3)
		{
			UE_LOG(LogGSplatEditor, Warning, TEXT("Export: %s has no import data - exporting in Unreal engine space instead of original capture space."), *InAsset.GetName());
			bUseOriginalSpace = false;
		}
		else
		{
			Swizzle = &ImportData->Swizzle;
			ScaleFactor = (FMath::Abs(ImportData->ScaleFactor) > KINDA_SMALL_NUMBER) ? ImportData->ScaleFactor : 1.0f;
			bInvertGamma = ImportData->bApplyGammaCorrection;
		}
	}

	// --- Determine which optional data will actually be written ---
	const uint32 NumHighOrderSH = FGaussianSplatData::NumHighOrderSH;

	const bool bWriteNormals = InOptions.bIncludeNormals && Splats.bHasNormals && Splats.Normals.Num() == SplatCount;
	if (InOptions.bIncludeNormals && !bWriteNormals)
	{
		UE_LOG(LogGSplatEditor, Warning, TEXT("Export: %s does not contain valid per-splat normals - omitting them from the file."), *InAsset.GetName());
	}

	const bool bWriteHighOrderSH = InOptions.bIncludeHighOrderSH && Splats.bHasHighOrderSH && Splats.HighOrderHarmonicsCoefficients.Num() == SplatCount * NumHighOrderSH;
	if (InOptions.bIncludeHighOrderSH && !bWriteHighOrderSH)
	{
		UE_LOG(LogGSplatEditor, Warning, TEXT("Export: %s does not contain valid high-order SH - omitting them from the file."), *InAsset.GetName());
	}

	const bool bWriteMetallicRoughness = InOptions.bIncludeMetallicRoughness && Splats.bHasMetallicRoughness && Splats.MetallicRoughness.Num() == SplatCount;
	if (InOptions.bIncludeMetallicRoughness && !bWriteMetallicRoughness)
	{
		UE_LOG(LogGSplatEditor, Warning, TEXT("Export: %s does not contain valid metallic/roughness - omitting it from the file."), *InAsset.GetName());
	}

	// --- Build the per-property data in standard 3DGS encodings ---
	std::vector<float> X(SplatCount), Y(SplatCount), Z(SplatCount);
	std::vector<float> FDC0(SplatCount), FDC1(SplatCount), FDC2(SplatCount);
	std::vector<float> Opacity(SplatCount);
	std::vector<float> Scale0(SplatCount), Scale1(SplatCount), Scale2(SplatCount);
	std::vector<float> Rot0(SplatCount), Rot1(SplatCount), Rot2(SplatCount), Rot3(SplatCount);
	std::vector<float> Nx, Ny, Nz;
	if (bWriteNormals)
	{
		Nx.resize(SplatCount);
		Ny.resize(SplatCount);
		Nz.resize(SplatCount);
		
	}
	std::vector<float> Metallic, Roughness;
	if (bWriteMetallicRoughness)
	{
		Metallic.reserve(SplatCount);
		Roughness.reserve(SplatCount);
	}
	std::vector<float> FRest;
	if (bWriteHighOrderSH)
	{
		FRest.resize(SplatCount * NumHighOrderSH);
	}

	const int32 ProgressChunk = 100000;

	for (uint32 i = 0; i < SplatCount; i++)
	{
		if (InProgress && (i % ProgressChunk) == 0)
		{
			InProgress->EnterProgressFrame(1, FText::Format(LOCTEXT("ExportingSplat", "Exporting {0}..."), { FText::FromString(InAsset.GetName()) }));
		}

		// Position
		FVector3f P = Splats.Positions[i];
		if (bUseOriginalSpace)
		{
			P = InverseSwizzleVector(P, *Swizzle) / ScaleFactor;
		}
		X[i] = P.X;
		Y[i] = P.Y;
		Z[i] = P.Z;

		// Base colour: always written as zero-order SH coefficients (standard 3DGS convention)
		FVector4f C = Splats.ZeroOrderHarmonicsCoefficientsAndOpacities[i];
		if (bInvertGamma)
		{
			// Undo the gamma correction applied on import so the file matches the original encoding
			// (clamped first because pow of a negative value is undefined)
			C.X = FMath::Pow(FMath::Max(C.X, 0.0f), 1.0f / GammaExponent);
			C.Y = FMath::Pow(FMath::Max(C.Y, 0.0f), 1.0f / GammaExponent);
			C.Z = FMath::Pow(FMath::Max(C.Z, 0.0f), 1.0f / GammaExponent);
		}

		FDC0[i] = ColorToSHDC(C.X);
		FDC1[i] = ColorToSHDC(C.Y);
		FDC2[i] = ColorToSHDC(C.Z);

		// Opacity: always float, pre-sigmoid (standard 3DGS convention)
		Opacity[i] = LogitOpacity(C.W);

		// Scale (stored in log space by convention)
		FVector3f S = Splats.Scales[i];
		if (bUseOriginalSpace)
		{
			S = InverseSwizzleVector(S, *Swizzle);
		}
		Scale0[i] = LogScale(S.X);
		Scale1[i] = LogScale(S.Y);
		Scale2[i] = LogScale(S.Z);

		// Rotation (stored as normalized wxyz quaternion by convention)
		const FQuat4f Q = Splats.Orientations[i];
		const FVector4f OutQ = bUseOriginalSpace
			? InverseSwizzleQuaternion(Q, *Swizzle)
			: FVector4f(Q.X, Q.Y, Q.Z, Q.W);

		Rot0[i] = OutQ.W;
		Rot1[i] = OutQ.X;
		Rot2[i] = OutQ.Y;
		Rot3[i] = OutQ.Z;

		// Normals: always float nx, ny, nz (standard 3DGS convention)
		if (bWriteNormals)
		{
			FVector3f N = Splats.Normals[i];
			if (bUseOriginalSpace)
			{
				N = InverseSwizzleVector(N, *Swizzle);
			}

			N = N.GetSafeNormal();
			Nx[i] = N.X;
			Ny[i] = N.Y;
			Nz[i] = N.Z;
		}

		// High-order SH (already in standard f_rest_0..44 order)
		if (bWriteHighOrderSH)
		{
			const float* Src = &Splats.HighOrderHarmonicsCoefficients[i * NumHighOrderSH];
			FMemory::Memcpy(&FRest[i * NumHighOrderSH], Src, NumHighOrderSH * sizeof(float));
		}

		// Metallic/roughness (linear 0..1 by convention)
		if (bWriteMetallicRoughness)
		{
			Metallic[i] = FMath::Clamp(Splats.MetallicRoughness[i].X, 0.0f, 1.0f);
			Roughness[i] = FMath::Clamp(Splats.MetallicRoughness[i].Y, 0.0f, 1.0f);
		}
	}

	// --- Assemble and write the PLY file ---
	try
	{
		happly::PLYData PlyOut;
		PlyOut.addElement("vertex", SplatCount);
		happly::Element& Vertices = PlyOut.getElement("vertex");

		Vertices.addProperty<float>("x", X);
		Vertices.addProperty<float>("y", Y);
		Vertices.addProperty<float>("z", Z);

		if (bWriteNormals)
		{
			Vertices.addProperty<float>("nx", Nx);
			Vertices.addProperty<float>("ny", Ny);
			Vertices.addProperty<float>("nz", Nz);
		}

		// Base colour: zero-order SH (standard 3DGS convention)
		Vertices.addProperty<float>("f_dc_0", FDC0);
		Vertices.addProperty<float>("f_dc_1", FDC1);
		Vertices.addProperty<float>("f_dc_2", FDC2);

		// Opacity: float, pre-sigmoid (standard 3DGS convention)
		Vertices.addProperty<float>("opacity", Opacity);

		Vertices.addProperty<float>("scale_0", Scale0);
		Vertices.addProperty<float>("scale_1", Scale1);
		Vertices.addProperty<float>("scale_2", Scale2);

		Vertices.addProperty<float>("rot_0", Rot0);
		Vertices.addProperty<float>("rot_1", Rot1);
		Vertices.addProperty<float>("rot_2", Rot2);
		Vertices.addProperty<float>("rot_3", Rot3);

		if (bWriteHighOrderSH)
		{
			for (uint32 k = 0; k < NumHighOrderSH; k++)
			{
				std::vector<float> FRestChannel(SplatCount);
				for (uint32 i = 0; i < SplatCount; i++)
				{
					FRestChannel[i] = FRest[i * NumHighOrderSH + k];
				}
				Vertices.addProperty<float>(("f_rest_" + std::to_string(k)).c_str(), FRestChannel);
			}
		}

		if (bWriteMetallicRoughness)
		{
			Vertices.addProperty<float>("metallic", Metallic);
			Vertices.addProperty<float>("roughness", Roughness);
		}

		PlyOut.write(std::string(TCHAR_TO_UTF8(*InFilename)), happly::DataFormat::Binary);
	}
	catch (const std::runtime_error& e)
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Export failed: %s"), UTF8_TO_TCHAR(e.what()));
		return false;
	}

	UE_LOG(LogGSplatEditor, Log, TEXT("Exported %s (%u splats) to %s"), *InAsset.GetName(), SplatCount, *InFilename);
	return true;
}


void FGaussianSplatDataExtension::RegisterMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.GSplatAsset");

	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
	Section.AddDynamicEntry("GetAssetActions_GSplatAsset", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			const TAttribute<FText> Label = LOCTEXT("GSplatAsset_ExportPLY", "Export PLY");
			const TAttribute<FText> ToolTip = LOCTEXT("GSplatAsset_ExportPLYToolTip", "Export Gaussian Splat Asset as PLY file.");
			const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Toolbar.Export");
			const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateStatic(&FGaussianSplatDataExtension::ExportPLY);

			InSection.AddMenuEntry("GSplatData_ExportPLY", Label, ToolTip, Icon, UIAction);
		}));
}

void FGaussianSplatDataExtension::ExportPLY(const FToolMenuContext& MenuContext)
{
	if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
	{
		for (UGSplatAsset* Asset : Context->LoadSelectedObjects<UGSplatAsset>())
		{
			if (!Asset)
			{
				UE_LOG(LogGSplatEditor, Error, TEXT("Export to PLY: the selected object is not a Gaussian Splat Data asset."));
				return;
			}

			if (Asset->Splats.SplatCount == 0)
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					LOCTEXT("ExportEmptyAssetText", "This splat asset contains no splats and cannot be exported."),
					LOCTEXT("ExportEmptyAssetTitle", "Cannot Export"));
				return;
			}

			TSharedPtr<SWindow> ParentWindow;
			if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
			{
				IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
				ParentWindow = MainFrame.GetParentWindow();
			}

			// Show the export options window
			constexpr float ExportWindowWidth = 550.0f;
			constexpr float ExportWindowHeight = 700.0f;

			TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(LOCTEXT("GSplatExportOptionsTitle", "Gaussian Splat Export Options"))
				.SizingRule(ESizingRule::Autosized)
				.AutoCenter(EAutoCenter::PrimaryWorkArea)
				.ClientSize(FVector2D(ExportWindowWidth, ExportWindowHeight));

			TSharedPtr<SGSplatExportWindow> ExportWindow;
			Window->SetContent(
				SAssignNew(ExportWindow, SGSplatExportWindow)
				.Asset(Asset)
				.WidgetWindow(Window)
				.MaxWindowHeight(ExportWindowHeight)
				.MaxWindowWidth(ExportWindowWidth)
			);

			FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

			if (!ExportWindow->ShouldExport())
			{
				return;
			}

			const FGSplatExportOptions Options = ExportWindow->GetExportOptions();

			// Pick the output file (default to the asset's own folder)
			FString DefaultPath = FPaths::ProjectContentDir();
			const FString PackagePath = FPackageName::LongPackageNameToFilename(Asset->GetPackage()->GetName());
			if (!PackagePath.IsEmpty())
			{
				DefaultPath = FPaths::GetPath(PackagePath);
			}
			FString DefaultFilename = FPaths::GetCleanFilename(Asset->GetName()) + TEXT(".ply");

			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			if (!DesktopPlatform)
			{
				return;
			}

			TArray<FString> Filenames;
			if (!DesktopPlatform->SaveFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				"Save PLY File",
				DefaultPath,
				DefaultFilename,
				"PLY Files|*.ply|All Files|*.*",
				EFileDialogFlags::None,
				Filenames))
			{
				// User cancelled the file dialog
				return;
			}

			if (Filenames.Num() == 0)
			{
				return;
			}
			const FString& Filename = Filenames[0];

			// Export with a progress bar (matching the chunking used in GSplatExporter)
			constexpr uint32 ExportProgressChunk = 100000;
			const uint32 NumChunks = (Asset->Splats.SplatCount + ExportProgressChunk - 1) / ExportProgressChunk;
			FScopedSlowTask SlowTask(static_cast<float>(NumChunks), LOCTEXT("ExportingSplatToPly", "Exporting Gaussian Splat to PLY..."));
			SlowTask.MakeDialog();

			if (GSplatExporter::ExportPly(*Asset, Options, Filename, &SlowTask))
			{
				FNotificationInfo NotificationInfo(FText::Format(
					LOCTEXT("ExportedNotification", "Exported {0} to {1}"),
					{ FText::FromName(Asset->GetFName()), FText::FromString(Filename) }));
				NotificationInfo.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(NotificationInfo);
			}
			else
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					LOCTEXT("ExportFailedText", "Failed to export splat to PLY. See output log for details."),
					LOCTEXT("ExportFailedTitle", "PLY Export Failed"));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
