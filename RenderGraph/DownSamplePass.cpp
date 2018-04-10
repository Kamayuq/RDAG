#include "DownSamplePass.h"


typename DownsampleRenderPass::PassOutputType DownsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DownSampleInfo = Input.GetDescriptor<RDAG::DownsampleInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	return Seq
	{
		Builder.CreateResource<RDAG::DownsampleResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction<RDAG::DownsampleResult>("DownsampleRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("DownsampleRenderAction");
		})
	}(Input);
}