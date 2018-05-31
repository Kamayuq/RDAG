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
	using ShadowMapRenderInput = ResourceTable<>;
	using ShadowMapRenderResult = ResourceTable<RDAG::ShadowMapTextureArray>;

	static ShadowMapRenderResult Build(const RenderPassBuilder& Builder, const ShadowMapRenderInput& Input, const SceneViewInfo& ViewInfo);
};