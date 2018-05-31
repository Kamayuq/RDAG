#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(GbufferTexture);
}

struct GbufferRenderPass
{
	using GbufferRenderInput = ResourceTable<RDAG::DepthTarget>;
	using GbufferRenderResult = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTarget>;

	static GbufferRenderResult Build(const RenderPassBuilder& Builder, const GbufferRenderInput& Input);
};