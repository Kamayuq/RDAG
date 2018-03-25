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

		void OnExecute(ImmediateRenderContext&, const DepthTexture::ResourceType& Resource) const;
	};
}


struct AmbientOcclusionPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::SceneViewInfo, RDAG::GbufferTarget, RDAG::DepthTexture>,
		OutputTable<RDAG::AmbientOcclusionTexture>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};