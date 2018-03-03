#include "SimpleBlendPass.h"

template<typename RenderContextType>
typename SimpleBlendPass<RenderContextType>::PassOutputType SimpleBlendPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto BlendSrcInfo = Input.GetInputDescriptor<RDAG::BlendSource>(0);
	Texture2d::Descriptor BlendDstDescriptor;
	BlendDstDescriptor.Name = "BlendDestinationRenderTarget";
	BlendDstDescriptor.Format = BlendSrcInfo.Format;
	BlendDstDescriptor.Height = BlendSrcInfo.Height;
	BlendDstDescriptor.Width = BlendSrcInfo.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::BlendDest>({ BlendDstDescriptor }),
		Builder.QueueRenderAction("SimpleBlendAction", [](RenderContextType&, const PassOutputType&)
		{
			//(void)Config;
			//auto BlendInput0 = FDataSet::GetStaticResource<RDAG::FBlendSource>(RndCtx, Self->PassData, 0);
			//(void)BlendInput0;
			//auto BlendInput1 = FDataSet::GetStaticResource<RDAG::FBlendSource>(RndCtx, Self->PassData, 1);
			//(void)BlendInput1;
			//auto BlendOutput = FDataSet::GetMutableResource<RDAG::FBlendDest>(RndCtx, Self->PassData);
			//(void)BlendOutput;

			//RndCtx.SetRenderPass(Self);
			////SetBlendMode 
			//RndCtx.BindStatic(BlendInput0);
			//RndCtx.BindStatic(BlendInput1);
			//RndCtx.BindMutable(BlendOutput);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template SimpleBlendPass<RenderContext>::PassOutputType SimpleBlendPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template SimpleBlendPass<ParallelRenderContext>::PassOutputType SimpleBlendPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template SimpleBlendPass<VulkanRenderContext>::PassOutputType SimpleBlendPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
