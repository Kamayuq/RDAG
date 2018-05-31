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
	using DepthRenderInput = ResourceTable<>;
	using DepthRenderResult = ResourceTable<RDAG::DepthTarget>;

	static DepthRenderResult Build(const RenderPassBuilder& Builder, const DepthRenderInput& Input, const SceneViewInfo& ViewInfo);
};