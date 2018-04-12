#include "BilateralUpsample.h"


typename BilateralUpsampleRenderPass::PassOutputType BilateralUpsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthTarget = Input.GetDescriptor<RDAG::DepthTexture>();
	auto ColorTarget = Input.GetDescriptor<RDAG::HalfResInput>();
	Texture2d::Descriptor UpsampleDescriptor;
	UpsampleDescriptor.Name = "UpsampleResult";
	UpsampleDescriptor.Format = ColorTarget.Format;
	UpsampleDescriptor.Height = DepthTarget.Height;
	UpsampleDescriptor.Width = DepthTarget.Width;

	return Seq
	{
		Builder.CreateResource<RDAG::UpsampleResult>({ UpsampleDescriptor }),
		Builder.QueueRenderAction("BilateralUpsampleAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("BilateralUpsampleAction");
		})
	}(Input);
}