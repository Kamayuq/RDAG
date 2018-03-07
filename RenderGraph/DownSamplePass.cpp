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
	(
		Builder.CreateOutputResource<RDAG::DownsampleResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto DownsampleInput = FDataSet::GetStaticResource<RDAG::FDownsampleInput>(RndCtx, Self->PassData);
			//(void)DownsampleInput;
			//auto DownsampleOutput = FDataSet::GetMutableResource<RDAG::FDownsampleResult>(RndCtx, Self->PassData);
			//(void)DownsampleOutput;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindStatic(DownsampleInput);
			//RndCtx.BindMutable(DownsampleOutput);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}