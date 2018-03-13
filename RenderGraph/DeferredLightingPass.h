#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ShadowMapPass.h"
#include "AmbientOcclusion.h"

namespace RDAG
{
	struct LightingResult : Texture2dResourceHandle<LightingResult>
	{
		static constexpr const char* Name = "LightingResult";

		explicit LightingResult() {}
	};
}


struct DeferredLightingPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTexture, RDAG::Gbuffer, RDAG::AmbientOcclusionResult, RDAG::SceneViewInfo, RDAG::ShadowMapTextureArray>,
		OutputTable<RDAG::LightingResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};