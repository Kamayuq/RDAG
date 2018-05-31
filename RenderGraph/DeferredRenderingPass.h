#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"
#include "PostprocessingPass.h"

struct DeferredRendererPass
{
	using DeferredRendererInput = ResourceTable<>;
	using DeferredRendererOutput = ResourceTable<RDAG::PostProcessingResult>;

	static DeferredRendererOutput Build(const RenderPassBuilder& Builder, const DeferredRendererInput& Input, const SceneViewInfo& ViewInfo);
};
