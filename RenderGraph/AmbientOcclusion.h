#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(AmbientOcclusionTexture);
}

struct AmbientOcclusionPass
{
	using PassInputType = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::AmbientOcclusionTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};