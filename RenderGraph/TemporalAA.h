#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "VelocityPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(TemporalAAInput);
	EXTERNAL_TEX_HANDLE(TemporalAAOutput);
}

struct TemporalAARenderPass
{
	using PassInputType = ResourceTable<RDAG::TemporalAAInput, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::TemporalAAOutput>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};