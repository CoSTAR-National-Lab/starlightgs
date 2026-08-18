#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGaussiansRenderer, Log, All)


namespace Gaussians
{
	// Ability to force-disable all relighting via CVar (mostly for demonstration purposes)
	GAUSSIANSRENDERER_API bool IsRelightingEnabled();
	GAUSSIANSRENDERER_API bool IsRelightingEnabled_RenderThread();

	GAUSSIANSRENDERER_API bool IsShadowsEnabled();
	GAUSSIANSRENDERER_API bool IsShadowsEnabled_RenderThread();
}


class FGaussiansRendererModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
