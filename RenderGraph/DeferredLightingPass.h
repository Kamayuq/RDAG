#pragma once
#include "Renderpass.h"
#include "ExampleResourceTypes.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ShadowMapPass.h"
#include "AmbientOcclusion.h"

namespace RDAG
{
	struct LightingUAV : Uav2dResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "LightingUAV";

		explicit LightingUAV() {}
	};
}


struct DeferredLightingPass
{
	using PassInputType = ResourceTable<RDAG::ShadowMapTextureArray, RDAG::AmbientOcclusionTexture, RDAG::GbufferTexture, RDAG::DepthTexture, RDAG::SceneViewInfo>;
	using PassOutputType = ResourceTable<RDAG::LightingUAV>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};