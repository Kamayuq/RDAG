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
	using VelocityRenderInput = ResourceTable<RDAG::DepthTexture>;
	using VelocityRenderResult = ResourceTable<RDAG::VelocityVectors>;

	static VelocityRenderResult Build(const RenderPassBuilder& Builder, const VelocityRenderInput& Input);
};