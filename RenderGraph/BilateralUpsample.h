#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(UpsampleResult, UpsampleResult);
	SIMPLE_TEX_HANDLE(HalfResInput);
	SIMPLE_TEX_HANDLE(HalfResDepth);
}


struct BilateralUpsampleRenderPass
{
	using PassInputType = ResourceTable<RDAG::HalfResInput, RDAG::HalfResDepth, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::UpsampleResult>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};