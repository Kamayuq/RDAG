#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "ForwardPass.h"

namespace RDAG
{
	SIMPLE_RT_HANDLE(TransparencyTarget, SceneColorTexture);
}

struct TransparencyRenderPass
{
	using PassInputType = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};