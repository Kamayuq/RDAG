#pragma once
#include "Renderpass.h"
#include "DepthPass.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(UpsampleResult, UpsampleResult);
	SIMPLE_TEX_HANDLE(HalfResInput);
	DEPTH_TEX_HANDLE(HalfResDepth);
}


struct BilateralUpsampleRenderPass
{
	using BilateralUpsampleInput = ResourceTable<RDAG::HalfResInput, RDAG::HalfResDepth, RDAG::DepthTexture>;
	using BilateralUpsampleResult = ResourceTable<RDAG::UpsampleResult>;

	static BilateralUpsampleResult Build(const RenderPassBuilder& Builder, const BilateralUpsampleInput& Input);
};