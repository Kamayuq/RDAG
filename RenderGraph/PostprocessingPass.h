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
	using ToneMappingInput = ResourceTable<RDAG::PostProcessingInput>;
	using ToneMappingResult = ResourceTable<RDAG::PostProcessingResult>;

	static ToneMappingResult Build(const RenderPassBuilder& Builder, const ToneMappingInput& Input);
};

struct PostProcessingPass
{
	using PostProcessingPassInput = ResourceTable<RDAG::PostProcessingInput, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using PostProcessingPassResult = ResourceTable<RDAG::PostProcessingResult>;

	static PostProcessingPassResult Build(const RenderPassBuilder& Builder, const PostProcessingPassInput& Input, const SceneViewInfo& ViewInfo);
};