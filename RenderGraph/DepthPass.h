#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "SharedResources.h"

namespace RDAG
{
	DEPTH_TEX_HANDLE(DepthTexture);
	DEPTH_RT_HANDLE(DepthTarget, DepthTexture);
}


struct DepthRenderPass
{
	using PassInputType = ResourceTable<>;
	using PassOutputType = ResourceTable<RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};