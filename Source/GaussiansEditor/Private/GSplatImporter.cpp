#include "GSplatImporter.h"
#include "GSplatFactory.h"

#if WITH_EDITOR

#include "GaussianData.h"
#include "GaussiansEditorModule.h"

#include <happly.h>


bool FGSplatImportData::ParsePly(const FString& Filename, FGSplatImportData& ImportData)
{
	try
	{
		std::string file = std::string(TCHAR_TO_UTF8(*Filename));
		ImportData.PlyData = new happly::PLYData(file);
	}
	catch (std::runtime_error e)
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Exception raised: %hs"), e.what())
			return false;
	}

	check(ImportData.PlyData);
	happly::PLYData& ply = *ImportData.PlyData;

	// Check ply has required data
	if (!ply.hasElement("vertex"))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Ply did not contain any vertices."))
			return false;
	}
	auto& vertices = ply.getElement("vertex");
	if (vertices.count == 0)
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Ply did not contain any vertices."))
			return false;
	}

	if (!(vertices.hasPropertyType<float>("x")
		&& vertices.hasPropertyType<float>("y")
		&& vertices.hasPropertyType<float>("z")))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Failed to find required field (position). Expected float fields x, y, z."))
			return false;
	}

	if (!((vertices.hasPropertyType<float>("f_dc_0")
		&& vertices.hasPropertyType<float>("f_dc_1")
		&& vertices.hasPropertyType<float>("f_dc_2"))
		|| (vertices.hasPropertyType<uint8_t>("red")
			&& vertices.hasPropertyType<uint8_t>("green")
			&& vertices.hasPropertyType<uint8_t>("blue"))))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Failed to find required field (color). Expected [float field f_dc_0, f_dc_1, f_dc_2] or [uint8 field red, green, blue]."))
			return false;
	}

	if (!(vertices.hasProperty("opacity") || vertices.hasPropertyType<uint8_t>("opacity")))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Failed to find required field (opacity). Expected [float field opacity] or [uint8 field opacity]."))
			return false;
	}

	if (!(vertices.hasPropertyType<float>("scale_0")
		&& vertices.hasPropertyType<float>("scale_1")
		&& vertices.hasPropertyType<float>("scale_2")))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Failed to find required field (scale). Expected float fields scale_0, scale_0, scale_0."))
			return false;
	}

	if (!(vertices.hasPropertyType<float>("rot_0")
		&& vertices.hasPropertyType<float>("rot_1")
		&& vertices.hasPropertyType<float>("rot_2")
		&& vertices.hasPropertyType<float>("rot_3")))
	{
		UE_LOG(LogGSplatEditor, Error, TEXT("Failed to import splat: Failed to find required field (rotation). Expected float fields rot_0, rot_1, rot_2, rot_3."))
			return false;
	}

	// Check support for optional properties
	ImportData.bHasNormals = (vertices.hasPropertyType<float>("nx") && vertices.hasPropertyType<float>("ny") && vertices.hasPropertyType<float>("nz"))
		|| (vertices.hasPropertyType<uint8_t>("octa_nx") && vertices.hasPropertyType<uint8_t>("octa_ny"));

	// Skipping high-order SH for now
	ImportData.bHasHighOrderSH = false;

	ImportData.bHasMetallicRoughness = (vertices.hasProperty("metallic") || vertices.hasProperty("metallicFactor"))
		&& (vertices.hasProperty("roughness") || vertices.hasProperty("roughnessFactor"));

	return true;
}

float SwizzleComponent(float InX, float InY, float InZ, EGSplatSwizzleAxis Swizzle)
{
	switch (Swizzle)
	{
	case EGSplatSwizzleAxis::X:	 return  InX;
	case EGSplatSwizzleAxis::Y:	 return  InY;
	case EGSplatSwizzleAxis::Z:	 return  InZ;
	case EGSplatSwizzleAxis::Xn: return -InX;
	case EGSplatSwizzleAxis::Yn: return -InY;
	case EGSplatSwizzleAxis::Zn: return -InZ;
	default:
		checkNoEntry();
	}
	return 0.0f;
}

FVector3f SwizzleVector(float InX, float InY, float InZ, const TStaticArray<EGSplatSwizzleAxis, 3>& Swizzle)
{
	return {
		SwizzleComponent(InX, InY, InZ, Swizzle[0]),
		SwizzleComponent(InX, InY, InZ, Swizzle[1]),
		SwizzleComponent(InX, InY, InZ, Swizzle[2])
	};
}

FVector3f SwizzleVector(const FVector3f& V, const TStaticArray<EGSplatSwizzleAxis, 3>& Swizzle)
{
	return {
		SwizzleComponent(V.X, V.Y, V.Z, Swizzle[0]),
		SwizzleComponent(V.X, V.Y, V.Z, Swizzle[1]),
		SwizzleComponent(V.X, V.Y, V.Z, Swizzle[2])
	};
}

float SwizzleComponentAbs(float InX, float InY, float InZ, EGSplatSwizzleAxis Swizzle)
{
	switch (Swizzle)
	{
	case EGSplatSwizzleAxis::X:
	case EGSplatSwizzleAxis::Xn: return  InX;
	case EGSplatSwizzleAxis::Y:
	case EGSplatSwizzleAxis::Yn: return  InY;
	case EGSplatSwizzleAxis::Z:
	case EGSplatSwizzleAxis::Zn: return  InZ;
	default:
		checkNoEntry();
	}
	return 0.0f;
}

FVector3f SwizzleVectorAbs(float InX, float InY, float InZ, const TStaticArray<EGSplatSwizzleAxis, 3>& Swizzle)
{
	return {
		SwizzleComponentAbs(InX, InY, InZ, Swizzle[0]),
		SwizzleComponentAbs(InX, InY, InZ, Swizzle[1]),
		SwizzleComponentAbs(InX, InY, InZ, Swizzle[2])
	};
}

FVector3f UnitVectorForAxis(EGSplatSwizzleAxis Axis)
{
	switch (Axis)
	{
	case EGSplatSwizzleAxis::X:	 return  FVector3f::UnitX();
	case EGSplatSwizzleAxis::Y:	 return  FVector3f::UnitY();
	case EGSplatSwizzleAxis::Z:	 return  FVector3f::UnitZ();
	case EGSplatSwizzleAxis::Xn: return -FVector3f::UnitX();
	case EGSplatSwizzleAxis::Yn: return -FVector3f::UnitY();
	case EGSplatSwizzleAxis::Zn: return -FVector3f::UnitZ();
	default:
		checkNoEntry();
	}
	return FVector3f::ZeroVector;
}

FVector4f SwizzleQuaternion(float InX, float InY, float InZ, float InW, const TStaticArray<EGSplatSwizzleAxis, 3>& Swizzle)
{
	FVector3f XAxis = UnitVectorForAxis(Swizzle[0]);
	FVector3f YAxis = UnitVectorForAxis(Swizzle[1]);
	FVector3f ZAxis = UnitVectorForAxis(Swizzle[2]);
	FVector3f XCrossY = XAxis ^ YAxis;
	bool bRightHanded = (XCrossY | ZAxis) > 0;

	return FVector4f{
		SwizzleComponent(InX, InY, InZ, Swizzle[0]),
		SwizzleComponent(InX, InY, InZ, Swizzle[1]),
		SwizzleComponent(InX, InY, InZ, Swizzle[2]),
		bRightHanded ? -InW : InW
	};
}

bool FGSplatImportData::ImportPly(const FGSplatImportData& ImportData, FGaussianSplatData& Splats)
{
	if (!ImportData.PlyData || !ImportData.PlyData->hasElement("vertex"))
	{
		return false;
	}
	auto& vertices = ImportData.PlyData->getElement("vertex");

	Splats.bHasNormals = ImportData.bImportNormals;

	// Skipping high-order SH for now
	Splats.bHasHighOrderSH = false;

	Splats.bHasMetallicRoughness = ImportData.bImportMetallicRoughness;

	// Important to declare available optional fields before resizing
	// to ensure optional fields are also resized
	Splats.Resize(vertices.count);

	std::vector<float> x = vertices.getProperty<float>("x");
	std::vector<float> y = vertices.getProperty<float>("y");
	std::vector<float> z = vertices.getProperty<float>("z");

	bool bHasZeroOrderSH = false;
	std::vector<float> dc_x, dc_y, dc_z;
	std::vector<uint8_t> red, green, blue;
	if (vertices.hasProperty("f_dc_0"))
	{
		bHasZeroOrderSH = true;

		dc_x = vertices.getProperty<float>("f_dc_0");
		dc_y = vertices.getProperty<float>("f_dc_1");
		dc_z = vertices.getProperty<float>("f_dc_2");
	}
	else
	{
		bHasZeroOrderSH = false;

		red = vertices.getProperty<uint8_t>("red");
		green = vertices.getProperty<uint8_t>("green");
		blue = vertices.getProperty<uint8_t>("blue");
	}

	bool bCompressedOpacity = false;
	std::vector<float> opacity_float;
	std::vector<uint8_t> opacity_uchar;
	if (vertices.hasPropertyType<float>("opacity"))
	{
		bCompressedOpacity = false;
		opacity_float = vertices.getProperty<float>("opacity");
	}
	else
	{
		bCompressedOpacity = true;
		opacity_uchar = vertices.getProperty<uint8_t>("opacity");
	}

	std::vector<float> scale0 = vertices.getProperty<float>("scale_0");
	std::vector<float> scale1 = vertices.getProperty<float>("scale_1");
	std::vector<float> scale2 = vertices.getProperty<float>("scale_2");

	std::vector<float> rot0 = vertices.getProperty<float>("rot_0");
	std::vector<float> rot1 = vertices.getProperty<float>("rot_1");
	std::vector<float> rot2 = vertices.getProperty<float>("rot_2");
	std::vector<float> rot3 = vertices.getProperty<float>("rot_3");

	bool OctaEncodedNormals = false;
	std::vector<float> nx, ny, nz;
	std::vector<uint8_t> octa_nx, octa_ny;
	if (Splats.bHasNormals)
	{
		if (vertices.hasPropertyType<float>("nx"))
		{
			OctaEncodedNormals = false;
			nx = vertices.getProperty<float>("nx");
			ny = vertices.getProperty<float>("ny");
			nz = vertices.getProperty<float>("nz");
		}
		else
		{
			OctaEncodedNormals = true;
			octa_nx = vertices.getProperty<uint8_t>("octa_nx");
			octa_ny = vertices.getProperty<uint8_t>("octa_ny");
		}
	}

	bool bCompressedPBR = false;
	std::vector<float> metallic_float, roughness_float;
	std::vector<uint8_t> metallic_uchar, roughness_uchar;
	if (Splats.bHasMetallicRoughness)
	{
		std::string metallicProperty = vertices.hasProperty("metallic") ? "metallic" : "metallicFactor";
		std::string roughnessProperty = vertices.hasProperty("roughness") ? "roughness" : "roughnessFactor";
		if (vertices.hasPropertyType<float>(metallicProperty) && vertices.hasPropertyType<float>(roughnessProperty))
		{
			bCompressedPBR = false;
			metallic_float = vertices.getProperty<float>(metallicProperty);
			roughness_float = vertices.getProperty<float>(roughnessProperty);
		}
		else if (vertices.hasPropertyType<uint8_t>(metallicProperty) && vertices.hasPropertyType<uint8_t>(roughnessProperty))
		{
			bCompressedPBR = true;
			metallic_uchar = vertices.getProperty<uint8_t>(metallicProperty);
			roughness_uchar = vertices.getProperty<uint8_t>(roughnessProperty);
		}
		else
		{
			UE_LOG(LogGSplatEditor, Warning, TEXT("Splat contained metallic/roughness that were not either both float or both 8bit uint."))
				Splats.bHasMetallicRoughness = false;
		}
	}

	auto& positions = Splats.Positions;
	auto& scales = Splats.Scales;
	auto& orientations = Splats.Orientations;
	auto& sho = Splats.ZeroOrderHarmonicsCoefficientsAndOpacities;

	auto& normals = Splats.Normals;
	auto& metallicRoughness = Splats.MetallicRoughness;

	for (int i = 0; i < vertices.count; i++)
	{
		float ScaleFactor = ImportData.ScaleFactor;
		positions[i] = SwizzleVector(x[i], y[i], z[i], ImportData.Swizzle) * ScaleFactor;

		if (bHasZeroOrderSH)
		{
			// * 0.282095 + 0.5 to RGB

			sho[i].X = dc_x[i]	* 0.28209479177387814f + 0.5;
			sho[i].Y = dc_y[i]	* 0.28209479177387814f + 0.5;
			sho[i].Z = dc_z[i]	* 0.28209479177387814f + 0.5;
		}
		else
		{
			auto UCharToFloat = [](uint8_t x)
				{
					// Convert to normalized float
					float f = static_cast<float>(x) / 255.0f;
					//return (f - 0.5f) / 0.28209479177387814f;
					return f;
				};
			sho[i].X = UCharToFloat(red[i]);
			sho[i].Y = UCharToFloat(green[i]);
			sho[i].Z = UCharToFloat(blue[i]);
		}

		if (bCompressedOpacity)
		{
			sho[i].W = static_cast<float>(opacity_uchar[i]) / 255.0f;
		}
		else
		{
			sho[i].W = 1.0f / (1.0f + FMath::Exp(-opacity_float[i]));
		}

		scales[i] = SwizzleVectorAbs(
			FMath::Exp(scale0[i]),
			FMath::Exp(scale1[i]),
			FMath::Exp(scale2[i]),
			ImportData.Swizzle);


		FVector4f SwizzledRot = SwizzleQuaternion(rot1[i], rot2[i], rot3[i], rot0[i], ImportData.Swizzle);
		orientations[i].X = SwizzledRot.X;
		orientations[i].Y = SwizzledRot.Y;
		orientations[i].Z = SwizzledRot.Z;
		orientations[i].W = SwizzledRot.W;
		orientations[i].Normalize();

		if (Splats.bHasNormals)
		{
			if (OctaEncodedNormals)
			{
				FVector2f f{
					static_cast<float>(octa_nx[i]) / 255.0f,
					static_cast<float>(octa_ny[i]) / 255.0f
				};

				// Ported from HLSL from:
				// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
				f = f * 2.0f - 1.0f;
				FVector3f n{ f.X, f.Y, 1.0f - FMath::Abs(f.X) - FMath::Abs(f.Y) };
				float t = FMath::Clamp(-n.Z, 0.0f, 1.0f);
				n.X += n.X >= 0.0 ? -t : t;
				n.Y += n.Y >= 0.0 ? -t : t;

				normals[i] = SwizzleVector(n, ImportData.Swizzle);
			}
			else
			{
				normals[i] = SwizzleVector(nx[i], ny[i], nz[i], ImportData.Swizzle);
			}

			normals[i].Normalize();
		}

		if (Splats.bHasMetallicRoughness)
		{
			if (bCompressedPBR)
			{
				metallicRoughness[i].X = static_cast<float>(metallic_uchar[i]) / 255.0f;
				metallicRoughness[i].Y = static_cast<float>(roughness_uchar[i]) / 255.0f;
			}
			else
			{
				metallicRoughness[i].X = metallic_float[i];
				metallicRoughness[i].Y = roughness_float[i];
			}
		}
	}

	Splats.SplatCount = vertices.count;

	Splats.RecalculateBounds();

	return true;
}

#endif


FGSplatImportData::~FGSplatImportData()
{
	if (PlyData)
	{
		delete PlyData;
		PlyData = nullptr;
	}
}
