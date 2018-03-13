#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	struct AmbientOcclusionResult : Texture2dResourceHandle<AmbientOcclusionResult>
	{
		static constexpr const char* Name = "AmbientOcclusionResult";

		explicit AmbientOcclusionResult() {}
	};
}


struct AmbientOcclusionPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo, RDAG::Gbuffer, RDAG::DepthTexture>,
		OutputTable<RDAG::AmbientOcclusionResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};