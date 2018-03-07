#pragma once
#include "Plumber.h"
#include "Renderpass.h"
#include "ResourceTypes.h"

namespace RDAG
{
	template<int Count>
	struct DownsamplePyramid;

	struct TemporalAAOutput;

	struct PostProcessingInput : Texture2dResourceHandle
	{
		static constexpr const char* Name = "PostProcessingInput";

		explicit PostProcessingInput() {}
		explicit PostProcessingInput(const TemporalAAOutput&) {}

		template<int I>
		explicit PostProcessingInput(const DownsamplePyramid<I>&) {}

		explicit PostProcessingInput(const Texture2dResourceHandle&) {}
	};

	struct PostProcessingResult : ExternalTexture2dResourceHandle
	{
		static constexpr const char* Name = "PostProcessingResult";

		explicit PostProcessingResult() {}

		explicit PostProcessingResult(const Texture2dResourceHandle&) {}
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