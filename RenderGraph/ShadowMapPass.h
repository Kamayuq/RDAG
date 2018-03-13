#pragma once
#include <vector>
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "SharedResources.h"

namespace RDAG
{
	struct DepthTarget;

	struct ShadowMapTextureArray : Texture2dResourceHandle<ShadowMapTextureArray>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "ShadowMapTextureArray";

		explicit ShadowMapTextureArray() {}
		explicit ShadowMapTextureArray(const DepthTarget&) {}
	};
}


struct ShadowMapRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo>,
		OutputTable<RDAG::ShadowMapTextureArray>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};