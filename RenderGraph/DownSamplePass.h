#pragma once
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "Renderpass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(DownsamplePyramid);
	SIMPLE_UAV_HANDLE(DownsampleResult, DownsampleResult);
	SIMPLE_TEX_HANDLE(DownsampleInput);
	DEPTH_UAV_HANDLE(DownsampleDepthResult, DownsampleDepthResult);
	DEPTH_TEX_HANDLE(DownsampleDepthInput);
}

struct DownsampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsampleResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

struct DownsampleDepthRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleDepthInput>;
	using PassOutputType = ResourceTable<RDAG::DownsampleDepthResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};

struct PyramidDownSampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::DownsampleInput>;
	using PassOutputType = ResourceTable<RDAG::DownsamplePyramid>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};