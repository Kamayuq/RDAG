#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
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
	using PassInputType = ResourceTable<RDAG::DepthTexture, RDAG::GbufferTexture, RDAG::AmbientOcclusionTexture, RDAG::SceneViewInfo, RDAG::ShadowMapTextureArray>;
	using PassOutputType = ResourceTable<RDAG::LightingUAV>;
	using PassActionType = ResourceTable<RDAG::LightingUAV, RDAG::DepthTexture, RDAG::GbufferTexture, RDAG::AmbientOcclusionTexture, RDAG::SceneViewInfo, RDAG::ShadowMapTextureArray>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};