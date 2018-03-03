#include "DownSamplePass.h"

template<typename RenderContextType>
typename DownsampleRenderPass<RenderContextType>::PassOutputType DownsampleRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
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
template DownsampleRenderPass<RenderContext>::PassOutputType DownsampleRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DownsampleRenderPass<ParallelRenderContext>::PassOutputType DownsampleRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DownsampleRenderPass<VulkanRenderContext>::PassOutputType DownsampleRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
