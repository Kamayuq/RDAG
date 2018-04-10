#pragma once
#include "Plumber.h"
#include "ResourceTypes.h"
#include "Renderpass.h"

namespace RDAG
{
	struct DepthTarget;
	struct PostProcessingInput;
	struct DownsampleResult;
	struct DownsampleInput;

	template<int Count>
	struct DownsamplePyramid : Texture2dResourceHandle<DownsamplePyramid<Count>>
	{
		static constexpr const U32 ResourceCount = Count;
		static constexpr const char* Name = "DownsamplePyramid";
		explicit DownsamplePyramid() {}
		explicit DownsamplePyramid(const DownsampleResult&) {}
		explicit DownsamplePyramid(const DownsampleInput&) {}
	};

	struct DownsampleResult : Texture2dResourceHandle<DownsampleResult>
	{
		static constexpr const char* Name = "DownsampleResult";
		explicit DownsampleResult() {}

		template<int I>
		explicit DownsampleResult(const DownsamplePyramid<I>&) {}
	};

	struct DownsampleInput : Texture2dResourceHandle<DownsampleInput>
	{
		static constexpr const char* Name = "DownsampleInput";
		explicit DownsampleInput() {}
		//explicit DownsampleInput(const PostProcessingInput&) {}
		//explicit DownsampleInput(const DepthTarget&) {}
		template<typename CRTP>
		explicit DownsampleInput(const Texture2dResourceHandle<CRTP>&) {}

		template<int I>
		explicit DownsampleInput(const DownsamplePyramid<I>&) {}
	};
}


struct DownsampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsampleResult>;
	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

template<typename InputType, typename OutputType = InputType>
auto RunDownsamplePass(const RenderPassBuilder& Builder, int InputOffset = 0, int OutputOffset = 0)
{
	return Seq
	{
		Builder.RenameEntry<InputType, RDAG::DownsampleInput>(InputOffset, 0),
		Builder.BuildRenderPass<RDAG::DownsampleResult>("DownsampleRenderPass", DownsampleRenderPass::Build),
		Builder.RenameEntry<RDAG::DownsampleResult, OutputType>(0, OutputOffset)
	};
}

template<int Count>
struct PyramidDownSampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsamplePyramid<Count>>;
	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input)
	{
		PassOutputType Output = Builder.RenameEntry<RDAG::DownsampleInput, RDAG::DownsamplePyramid<Count>>(0, 0)(Input);
		
		U32 i = 1;
		for (; i < RDAG::DownsamplePyramid<Count>::ResourceCount; i++)
		{
			auto DownSampleInfo = Output.template GetDescriptor<RDAG::DownsamplePyramid<Count>>(i - 1);
			if (((DownSampleInfo.Width >> 1) == 0) || ((DownSampleInfo.Height >> 1) == 0))
				break;

			Output = RunDownsamplePass<RDAG::DownsamplePyramid<Count>>(Builder, i - 1, i)(Output);
		}

		auto DownSampleInfo = Output.template GetDescriptor<RDAG::DownsamplePyramid<Count>>(i - 1);
		check((DownSampleInfo.Width == 1 || DownSampleInfo.Height == 1) && "Downsample incomplete due to insufficent space available");
		return Output;
	}
};