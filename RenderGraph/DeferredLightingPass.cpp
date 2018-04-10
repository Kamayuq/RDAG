#include "DeferredLightingPass.h"


typename DeferredLightingPass::PassOutputType DeferredLightingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetDescriptor<RDAG::DepthTexture>();
	Texture2d::Descriptor LightingDescriptor;
	LightingDescriptor.Name = "LightingRenderTarget";
	LightingDescriptor.Format = ERenderResourceFormat::ARGB16F;
	LightingDescriptor.Height = DepthInfo.Height;
	LightingDescriptor.Width = DepthInfo.Width;

	return Seq
	{
		Builder.CreateResource<RDAG::LightingUAV>({ LightingDescriptor }),
		Builder.QueueRenderAction("DeferredLightingAction", [](RenderContext& Ctx, const PassActionType& Resources) -> PassOutputType
		{
			Ctx.Draw("DeferredLightingAction");
			return Resources;
		})
	}(Input);
}