#include "BilateralUpsample.h"


typename BilateralUpsampleRenderPass::PassOutputType BilateralUpsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthTarget = Input.GetInputDescriptor<RDAG::DepthTexture>();
	auto ColorTarget = Input.GetInputDescriptor<RDAG::HalfResInput>();
	Texture2d::Descriptor UpsampleDescriptor;
	UpsampleDescriptor.Name = "UpsampleResult";
	UpsampleDescriptor.Format = ColorTarget.Format;
	UpsampleDescriptor.Height = DepthTarget.Height;
	UpsampleDescriptor.Width = DepthTarget.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::UpsampleResult>({ UpsampleDescriptor }),
		Builder.QueueRenderAction("BilateralUpsampleAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("BilateralUpsampleAction");
		})
	)(Input);
}