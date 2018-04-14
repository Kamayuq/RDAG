#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ShadowMapPass.h"
#include "AmbientOcclusion.h"

struct DeferredLightingPass
{
	using PassInputType = ResourceTable<RDAG::ShadowMapTextureArray, RDAG::AmbientOcclusionTexture, RDAG::GbufferTexture, RDAG::DepthTexture, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::SceneColorTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};