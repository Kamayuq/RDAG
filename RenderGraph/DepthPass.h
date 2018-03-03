#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "Plumber.h"
#include "SharedResources.h"

namespace RDAG
{
	struct DownsampleResult;

	struct DepthTarget : Texture2dResourceHandle
	{
		static constexpr const char* Name = "DepthTarget";
		explicit DepthTarget() {}
		explicit DepthTarget(const DownsampleResult&) {}
	};

	struct HalfResDepthTarget : Texture2dResourceHandle
	{
		static constexpr const char* Name = "HalfResDepthTarget";
		explicit HalfResDepthTarget() {}
	};
}

template<typename RenderContextType = RenderContext>
struct DepthRenderPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo>,
		OutputTable<RDAG::DepthTarget>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};