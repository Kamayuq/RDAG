#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "VelocityPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE2(PostProcessingInput, SceneColorTexture);
	EXTERNAL_UAV_HANDLE(PostProcessingResult, PostProcessingResult);
}

struct ToneMappingPass
{
	using PassInputType = ResourceTable<RDAG::PostProcessingInput>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

struct PostProcessingPass
{
	using PassInputType = ResourceTable<RDAG::PostProcessingInput, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};