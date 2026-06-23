// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGSplats, Log, All)


namespace Gaussians
{
	// Ability to force-disable all relighting via CVar (mostly for demonstration purposes)
	bool IsRelightingEnabled();
	bool IsRelightingEnabled_RenderThread();

	bool IsShadowsEnabled();
	bool IsShadowsEnabled_RenderThread();
}


class FGaussiansModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
