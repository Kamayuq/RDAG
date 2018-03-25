#pragma once
#include "Renderpass.h"
#include "ResourceTypes.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ShadowMapPass.h"
#include "AmbientOcclusion.h"

namespace RDAG
{
	struct LightingUAV : Texture2dResourceHandle<LightingUAV>
	{
		static constexpr const char* Name = "LightingUAV";

		explicit LightingUAV() {}

		void OnExecute(ImmediateRenderContext& Ctx, const LightingUAV::ResourceType& Resource) const
		{
			Ctx.TransitionResource(Resource, EResourceTransition::UAV);
			Ctx.BindRenderTarget(Resource);
		}
	};
}


struct DeferredLightingPass
{
	RESOURCE_TABLE
	(
		InputTable<RDAG::DepthTexture, RDAG::GbufferTarget, RDAG::AmbientOcclusionTexture, RDAG::SceneViewInfo, RDAG::ShadowMapTextureArray>,
		OutputTable<RDAG::LightingUAV>
	);

	static PassOutputType Build(const RenderPassBuilder& Builder, const PassInputType& Input);
};