#include "DepthPass.h"

template<typename RenderContextType>
typename DepthRenderPass<RenderContextType>::PassOutputType DepthRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor DepthDescriptor;
	DepthDescriptor.Name = "DepthRenderTarget";
	DepthDescriptor.Format = ViewInfo.DepthFormat;
	DepthDescriptor.Height = ViewInfo.SceneHeight;
	DepthDescriptor.Width = ViewInfo.SceneWidth;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::DepthTarget>({ DepthDescriptor }),
		Builder.QueueRenderAction("DepthRenderAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto DepthTarget = FDataSet::GetMutableResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template DepthRenderPass<RenderContext>::PassOutputType DepthRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DepthRenderPass<ParallelRenderContext>::PassOutputType DepthRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DepthRenderPass<VulkanRenderContext>::PassOutputType DepthRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
