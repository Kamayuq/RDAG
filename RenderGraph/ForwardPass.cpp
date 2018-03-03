#include "ForwardPass.h"

template<typename RenderContextType>
typename ForwardRenderPass<RenderContextType>::PassOutputType ForwardRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto ForwardInfo = Input.GetInputHandle<RDAG::ForwardRender>();
	(void)ForwardInfo;

	return Seq
	(
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContextType&, const PassOutputType&)
		{
			//(void)Config;
			//auto DepthTarget = FDataSet::GetMutableResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto ForwardTarget = FDataSet::GetMutableResource<RDAG::FForwardResult>(RndCtx, Self->PassData);
			//(void)ForwardTarget;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.BindMutable(ForwardTarget);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template ForwardRenderPass<RenderContext>::PassOutputType ForwardRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ForwardRenderPass<ParallelRenderContext>::PassOutputType ForwardRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ForwardRenderPass<VulkanRenderContext>::PassOutputType ForwardRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
