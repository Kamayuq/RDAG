#include "GbufferPass.h"

template<typename RenderContextType>
typename GbufferRenderPass<RenderContextType>::PassOutputType GbufferRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor GbufferDescriptors[RDAG::Gbuffer::ResourceCount];
	for (U32 i = 0; i < RDAG::Gbuffer::ResourceCount; i++)
	{
		GbufferDescriptors[i] = Input.GetInputDescriptor<RDAG::DepthTarget>();
		GbufferDescriptors[i].Name = "GbufferTarget";
		GbufferDescriptors[i].Format = ERenderResourceFormat::ARGB16F;
	}

	return Seq
	(
		Builder.CreateOutputResource<RDAG::Gbuffer>(GbufferDescriptors),
		Builder.QueueRenderAction("GbufferRenderAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto DepthTarget = FDataSet::GetMutableResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto GbufferA = FDataSet::GetMutableResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 0);
			//(void)GbufferA;
			//auto GbufferB = FDataSet::GetMutableResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 1);
			//(void)GbufferB;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.BindMutable(GbufferA);
			//RndCtx.BindMutable(GbufferB);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template GbufferRenderPass<RenderContext>::PassOutputType GbufferRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template GbufferRenderPass<ParallelRenderContext>::PassOutputType GbufferRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template GbufferRenderPass<VulkanRenderContext>::PassOutputType GbufferRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
