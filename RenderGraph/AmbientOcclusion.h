#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	struct AmbientOcclusionResult : Texture2dResourceHandle
	{
		static constexpr const char* Name = "AmbientOcclusionResult";

		explicit AmbientOcclusionResult() {}
	};
}

template<typename RenderContextType = RenderContext>
struct AmbientOcclusionPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo, RDAG::Gbuffer, RDAG::DepthTarget>,
		OutputTable<RDAG::AmbientOcclusionResult>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};