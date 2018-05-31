#include "DeferredLightingPass.h"

namespace RDAG
{
	SIMPLE_UAV_HANDLE(LightingUAV, SceneColorTexture);
}

typename DeferredLightingPass::DeferredLightingResult DeferredLightingPass::Build(const RenderPassBuilder& Builder, const DeferredLightingInput& Input)
{
	const Texture2d::Descriptor& DepthInfo = Input.GetDescriptor<RDAG::DepthTexture>();
	Texture2d::Descriptor LightingDescriptor;
	LightingDescriptor.Name = "LightingRenderTarget";
	LightingDescriptor.Format = ERenderResourceFormat::ARGB16F;
	LightingDescriptor.Height = DepthInfo.Height;
	LightingDescriptor.Width = DepthInfo.Width;

	using DeferredLightingAction = ResourceTable<RDAG::LightingUAV, RDAG::ShadowMapTextureArray, RDAG::AmbientOcclusionTexture, RDAG::GbufferTexture, RDAG::DepthTexture>;
	return Seq
	{
		Builder.CreateResource<RDAG::LightingUAV>({ LightingDescriptor }),
		Builder.QueueRenderAction("DeferredLightingAction", [](RenderContext& Ctx, const DeferredLightingAction&)
		{
			Ctx.Draw("DeferredLightingAction");
		})
	}(Input);
}