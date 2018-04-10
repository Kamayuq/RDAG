#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "SharedResources.h"
#include "PostprocessingPass.h"


struct DeferredRendererPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;
	using PassActionType = ResourceTable<RDAG::PostProcessingResult, RDAG::SceneViewInfo>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};
