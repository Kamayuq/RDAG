#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ShadowMapPass.h"
#include "AmbientOcclusion.h"

struct DeferredLightingPass
{
	using DeferredLightingInput = ResourceTable<RDAG::ShadowMapTextureArray, RDAG::AmbientOcclusionTexture, RDAG::GbufferTexture, RDAG::DepthTexture>;
	using DeferredLightingResult = ResourceTable<RDAG::SceneColorTexture>;

	static DeferredLightingResult Build(const RenderPassBuilder& Builder, const DeferredLightingInput& Input);
};