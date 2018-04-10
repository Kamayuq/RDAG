#pragma once
#include "Renderpass.h"
#include "Plumber.h"
#include "ResourceTypes.h"
#include "GbufferPass.h"

namespace RDAG
{
	struct AmbientOcclusionTexture : Texture2dResourceHandle<AmbientOcclusionTexture>
	{
		static constexpr const char* Name = "AmbientOcclusionTexture";

		explicit AmbientOcclusionTexture() {}
		explicit AmbientOcclusionTexture(const struct AmbientOcclusionUAV&) {}

		void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource) const;
	};
}


struct AmbientOcclusionPass
{
	using PassInputType = ResourceTable<RDAG::SceneViewInfo, RDAG::GbufferTarget, RDAG::DepthTexture>;
	using PassOutputType = ResourceTable<RDAG::AmbientOcclusionTexture>;
	using PassActionType = ResourceTable<RDAG::AmbientOcclusionTexture, RDAG::SceneViewInfo, RDAG::GbufferTarget, RDAG::DepthTexture>;

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};