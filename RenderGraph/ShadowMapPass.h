#pragma once
#include <vector>
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "SharedResources.h"

namespace RDAG
{
	struct ShadowMapTextureArray : Texture2dResourceHandle<ShadowMapTextureArray>
	{
		static constexpr const char* Name = "ShadowMapTextureArray";

		explicit ShadowMapTextureArray() {}
		explicit ShadowMapTextureArray(const struct DepthTarget&) {}
	};
}

struct ShadowMapRenderPass
{
	using PassInputType = ResourceTable<>;
	using PassOutputType = ResourceTable<RDAG::ShadowMapTextureArray>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};