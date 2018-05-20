#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(GbufferTexture);
}

struct GbufferRenderPass
{
	using PassInputType = ResourceTable<RDAG::DepthTarget>;
	using PassOutputType = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTarget>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};