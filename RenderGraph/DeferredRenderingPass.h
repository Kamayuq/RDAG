#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "SharedResources.h"
#include "PostprocessingPass.h"


struct DeferredRendererPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo>,
		OutputTable<RDAG::PostProcessingResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};
