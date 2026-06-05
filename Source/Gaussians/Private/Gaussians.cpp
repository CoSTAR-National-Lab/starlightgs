// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gaussians.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FGaussiansModule"

DEFINE_LOG_CATEGORY(LogGSplats);


void FGaussiansModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Gaussians"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/Gaussians"), PluginShaderDir);
	AddShaderSourceSharedVirtualDirectory(TEXT("/Plugin/Gaussians/Shared/"));
}

void FGaussiansModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGaussiansModule, Gaussians)