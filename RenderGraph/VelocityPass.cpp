#include "VelocityPass.h"

template<typename RenderContextType>
typename VelocityRenderPass<RenderContextType>::ReturnType VelocityRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetInputDescriptor<RDAG::DepthTarget>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

#if ASYNC_VELOCITY
	return Async
#else
	return Seq
#endif
	(
		Builder.CreateOutputResource<RDAG::VelocityVectors>({ VelocityDescriptor }),
		Builder.QueueRenderAction("VelocityRenderAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto DepthTarget = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto ForwardTarget = FDataSet::GetMutableResource<RDAG::FVelocityVectors>(RndCtx, Self->PassData);
			//(void)ForwardTarget;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.BindMutable(ForwardTarget);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template VelocityRenderPass<RenderContext>::ReturnType VelocityRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template VelocityRenderPass<ParallelRenderContext>::ReturnType VelocityRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template VelocityRenderPass<VulkanRenderContext>::ReturnType VelocityRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
