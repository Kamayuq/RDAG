#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "VelocityPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(TemporalAAInput);
	EXTERNAL_UAV_HANDLE(TemporalAAOutput, TemporalAAOutput);
}

struct TemporalAARenderPass
{
	using TemporalAARenderInput = ResourceTable<RDAG::TemporalAAInput, RDAG::VelocityVectors, RDAG::DepthTexture>;
	using TemporalAARenderResult = ResourceTable<RDAG::TemporalAAOutput>;

	static TemporalAARenderResult Build(const RenderPassBuilder& Builder, const TemporalAARenderInput& Input, const SceneViewInfo& ViewInfo, const char* TaaKey);
};