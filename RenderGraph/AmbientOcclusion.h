#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	SIMPLE_TEX_HANDLE(AmbientOcclusionTexture);
}

struct AmbientOcclusionPass
{
	using AmbientOcclusionInput = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTexture>;
	using AmbientOcclusionResult = ResourceTable<RDAG::AmbientOcclusionTexture>;

	static AmbientOcclusionResult Build(const RenderPassBuilder& Builder, const AmbientOcclusionInput& Input, const SceneViewInfo& ViewInfo);
};