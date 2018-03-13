#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"

namespace RDAG
{
	template<int Count>
	struct DownsamplePyramid;

	struct TemporalAAOutput;

	struct PostProcessingInput : Texture2dResourceHandle<PostProcessingInput>
	{
		static constexpr const char* Name = "PostProcessingInput";

		explicit PostProcessingInput() {}
		explicit PostProcessingInput(const TemporalAAOutput&) {}

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
	RESOURCE_TABLE
	(
		InputTable<RDAG::PostProcessingInput>,
		OutputTable<RDAG::PostProcessingResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};


struct PostProcessingPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::PostProcessingInput>,
		OutputTable<RDAG::PostProcessingResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};