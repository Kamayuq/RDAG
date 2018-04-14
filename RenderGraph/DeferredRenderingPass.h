#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"
#include "PostprocessingPass.h"

struct DeferredRendererPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};
