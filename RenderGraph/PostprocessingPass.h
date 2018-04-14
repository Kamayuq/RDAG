#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "VelocityPass.h"

namespace RDAG
{
	struct PostProcessingInput : Texture2dResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "PostProcessingInput";

		explicit PostProcessingInput() {}

		explicit PostProcessingInput(const struct DownsamplePyramid&) {}

		template<typename CRTP>
		explicit PostProcessingInput(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct PostProcessingResult : ExternalUav2dResourceHandle<PostProcessingResult>
	{
		static constexpr const char* Name = "PostProcessingResult";

		explicit PostProcessingResult() {}

		template<typename CRTP>
		explicit PostProcessingResult(const Texture2dResourceHandle<CRTP>&) {}
	};
}

struct ToneMappingPass
{
	using PassInputType = ResourceTable<RDAG::PostProcessingInput>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

struct PostProcessingPass
{
	using PassInputType = ResourceTable<RDAG::PostProcessingInput, RDAG::VelocityVectors, RDAG::DepthTexture, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};