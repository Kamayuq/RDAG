#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "VelocityPass.h"

namespace RDAG
{
	template<int Count>
	struct DownsamplePyramid;

	struct TemporalAAOutput;

	struct PostProcessingInput : Texture2dResourceHandle<PostProcessingInput>
	{
		static constexpr const char* Name = "PostProcessingInput";

		explicit PostProcessingInput() {}

		template<int I>
		explicit PostProcessingInput(const DownsamplePyramid<I>&) {}

		template<typename CRTP>
		explicit PostProcessingInput(const Texture2dResourceHandle<CRTP>&) {}
	};

	struct PostProcessingResult : ExternalTexture2dResourceHandle<PostProcessingResult>
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
	using PassActionType = ResourceTable<RDAG::PostProcessingResult, RDAG::PostProcessingInput>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};


struct PostProcessingPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo, RDAG::PostProcessingInput, RDAG::DepthTexture, RDAG::VelocityVectors>;
	using PassOutputType = ResourceTable<RDAG::PostProcessingResult>;
	using PassActionType = ResourceTable<RDAG::PostProcessingResult, RDAG::SceneViewInfo, RDAG::PostProcessingInput, RDAG::DepthTexture, RDAG::VelocityVectors>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};