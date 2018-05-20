#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(VelocityVectors);
}

struct VelocityRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::VelocityVectors>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};