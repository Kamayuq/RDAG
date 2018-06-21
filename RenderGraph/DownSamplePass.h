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
	using DownsampleRenderInput = ResourceTable<RDAG::DownsampleInput, RDAG::DownsampleResult>;
	using DownsampleRenderResult = ResourceTable<RDAG::DownsampleResult>;

	static DownsampleRenderResult Build(const RenderPassBuilder& Builder, const DownsampleRenderInput& Input);
};

struct DownsampleDepthRenderPass
{
	using DownsampleDepthInput = ResourceTable<RDAG::DownsampleDepthInput>;
	using DownsampleDepthResult = ResourceTable<RDAG::DownsampleDepthResult>;

	static DownsampleDepthResult Build(const RenderPassBuilder& Builder, const DownsampleDepthInput& Input);
};

struct PyramidDownSampleRenderPass
{
	using PyramidDownSampleRenderInput = ResourceTable<RDAG::DownsampleInput>;
	using PyramidDownSampleRenderResult = ResourceTable<RDAG::DownsamplePyramid>;

	static PyramidDownSampleRenderResult Build(const RenderPassBuilder& Builder, const PyramidDownSampleRenderInput& Input);
};