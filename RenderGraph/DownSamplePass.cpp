#include "DownSamplePass.h"


typename DownsampleRenderPass::PassOutputType DownsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DownSampleInfo = Input.GetInputDescriptor<RDAG::DownsampleInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	return Seq
	{
		Builder.CreateOutputResource<RDAG::DownsampleResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("DownsampleRenderAction");
		})
	}(Input);
}