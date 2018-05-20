#pragma once
#include <vector>
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(ShadowMapTextureArray);
}

struct ShadowMapRenderPass
{
	using PassInputType = ResourceTable<>;
	using PassOutputType = ResourceTable<RDAG::ShadowMapTextureArray>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};