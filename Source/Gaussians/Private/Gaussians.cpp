// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gaussians.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FGaussiansModule"

DEFINE_LOG_CATEGORY(LogGSplats);

static TAutoConsoleVariable<bool> CVarGaussiansRelighting(
	TEXT("r.Gaussians.Relighting"),
	true,
	TEXT("Global toggle for Gaussian relighting."),
	ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<bool> CVarGaussiansShadows(
	TEXT("r.Gaussians.Shadows"),
	true,
	TEXT("Global toggle for Gaussian shadow casting."),
	ECVF_RenderThreadSafe
);

namespace Gaussians
{

	bool IsRelightingEnabled()
	{
		return CVarGaussiansRelighting.GetValueOnAnyThread();
	}

	bool IsRelightingEnabled_RenderThread()
	{
		return CVarGaussiansRelighting.GetValueOnRenderThread();
	}

	bool IsShadowsEnabled()
	{
		return CVarGaussiansShadows.GetValueOnAnyThread();
	}

	bool IsShadowsEnabled_RenderThread()
	{
		return CVarGaussiansShadows.GetValueOnRenderThread();
	}

}


void FGaussiansModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("StarLightGS"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/StarLightGS"), PluginShaderDir);
	AddShaderSourceSharedVirtualDirectory(TEXT("/Plugin/StarLightGS/Shared/"));
}

void FGaussiansModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGaussiansModule, Gaussians)