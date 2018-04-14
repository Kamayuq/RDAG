#pragma once
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "Renderpass.h"

namespace RDAG
{
	struct DownsamplePyramid : Texture2dResourceHandle<DownsamplePyramid>
	{
		static constexpr const char* Name = "DownsamplePyramid";
		explicit DownsamplePyramid() {}
		explicit DownsamplePyramid(const struct DownsampleResult&) {}
		explicit DownsamplePyramid(const struct DownsampleInput&) {}
	};

	struct DownsampleResult : Uav2dResourceHandle<DownsampleResult>
	{
		static constexpr const char* Name = "DownsampleResult";
		explicit DownsampleResult() {}

		explicit DownsampleResult(const DownsamplePyramid&) {}
	};

	struct DownsampleInput : Texture2dResourceHandle<DownsampleInput>
	{
		static constexpr const char* Name = "DownsampleInput";
		explicit DownsampleInput() {}

		template<typename CRTP>
		explicit DownsampleInput(const Texture2dResourceHandle<CRTP>&) {}

		explicit DownsampleInput(const DownsamplePyramid&) {}
	};
}

struct DownsampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsampleResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

struct PyramidDownSampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsamplePyramid>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};