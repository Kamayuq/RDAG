#pragma once
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "Renderpass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(DownsamplePyramid);
	SIMPLE_UAV_HANDLE(DownsampleResult, DownsampleResult);
	SIMPLE_TEX_HANDLE(DownsampleInput);
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