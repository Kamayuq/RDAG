#include "DeferredLightingPass.h"

namespace RDAG
{
	struct LightingUAV : Uav2dResourceHandle<SceneColorTexture>
	{
		static constexpr const char* Name = "LightingUAV";

		explicit LightingUAV() {}
	};
}

typename DeferredLightingPass::PassOutputType DeferredLightingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetDescriptor<RDAG::DepthTexture>();
	Texture2d::Descriptor LightingDescriptor;
	LightingDescriptor.Name = "LightingRenderTarget";
	LightingDescriptor.Format = ERenderResourceFormat::ARGB16F;
	LightingDescriptor.Height = DepthInfo.Height;
	LightingDescriptor.Width = DepthInfo.Width;

	using PassActionType = ResourceTable<RDAG::LightingUAV, RDAG::ShadowMapTextureArray, RDAG::AmbientOcclusionTexture, RDAG::GbufferTexture, RDAG::DepthTexture>;
	return Seq
	{
		Builder.CreateResource<RDAG::LightingUAV>({ LightingDescriptor }),
		Builder.QueueRenderAction("DeferredLightingAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("DeferredLightingAction");
		})
	}(Input);
}