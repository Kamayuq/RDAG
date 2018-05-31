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
	using TransparencyRenderInput = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;
	using TransparencyRenderResult = ResourceTable<RDAG::TransparencyTarget, RDAG::DepthTarget>;

	static TransparencyRenderResult Build(const RenderPassBuilder& Builder, const TransparencyRenderInput& Input, const SceneViewInfo& ViewInfo);
};