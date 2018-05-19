#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ExampleResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	struct AmbientOcclusionTexture : Texture2dResourceHandle<AmbientOcclusionTexture>
	{
		static constexpr const char* Name = "AmbientOcclusionTexture";

		explicit AmbientOcclusionTexture() {}
		explicit AmbientOcclusionTexture(const struct AmbientOcclusionUAV&) {}
	};
}

struct AmbientOcclusionPass
{
	using PassInputType = ResourceTable<RDAG::GbufferTexture, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::AmbientOcclusionTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo);
};