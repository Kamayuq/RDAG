#include "BilateralUpsample.h"


typename BilateralUpsampleRenderPass::BilateralUpsampleResult BilateralUpsampleRenderPass::Build(const RenderPassBuilder& Builder, const BilateralUpsampleInput& Input)
{
	const Texture2d::Descriptor& DepthTarget = Input.GetDescriptor<RDAG::DepthTexture>();
	const Texture2d::Descriptor& ColorTarget = Input.GetDescriptor<RDAG::HalfResInput>();
	Texture2d::Descriptor UpsampleDescriptor;
	UpsampleDescriptor.Name = "UpsampleResult";
	UpsampleDescriptor.Format = ColorTarget.Format;
	UpsampleDescriptor.Height = DepthTarget.Height;
	UpsampleDescriptor.Width = DepthTarget.Width;

	using BilateralUpsampleAction = decltype(std::declval<BilateralUpsampleInput>().Union(std::declval<BilateralUpsampleResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::UpsampleResult>({ UpsampleDescriptor }),
		Builder.QueueRenderAction("BilateralUpsampleAction", [](RenderContext& Ctx, const BilateralUpsampleAction&)
		{
			Ctx.Draw("BilateralUpsampleAction");
		})
	}(Input);
}