#include "BilateralUpsample.h"

template<typename RenderContextType>
typename BilateralUpsampleRenderPass<RenderContextType>::PassOutputType BilateralUpsampleRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthTarget = Input.GetInputDescriptor<RDAG::DepthTarget>();
	auto ColorTarget = Input.GetInputDescriptor<RDAG::HalfResInput>();
	Texture2d::Descriptor UpsampleDescriptor;
	UpsampleDescriptor.Name = "UpsampleResult";
	UpsampleDescriptor.Format = ColorTarget.Format;
	UpsampleDescriptor.Height = DepthTarget.Height;
	UpsampleDescriptor.Width = DepthTarget.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::UpsampleResult>({ UpsampleDescriptor }),
		Builder.QueueRenderAction("BilateralUpsampleAction", [](RenderContextType&, const PassOutputType&)
		{
			//auto UpsampleInput = FDataSet::GetStaticResource<RDAG::FHalfResInput>(RndCtx, Self->PassData);
			//(void)UpsampleInput;
			//auto UpsampleInputHalfResDepth = FDataSet::GetStaticResource<RDAG::FHalfResDepth>(RndCtx, Self->PassData);
			//(void)UpsampleInputHalfResDepth;
			//auto UpsampleInputDepth = FDataSet::GetStaticResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)UpsampleInputDepth;
			//auto DownsampleOutput = FDataSet::GetMutableResource<RDAG::FUpsampleResult>(RndCtx, Self->PassData);
			//(void)DownsampleOutput;
		
			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindStatic(UpsampleInput);
			//RndCtx.BindStatic(UpsampleInputHalfResDepth);
			//RndCtx.BindStatic(UpsampleInputDepth);
			//RndCtx.BindMutable(DownsampleOutput);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}
template BilateralUpsampleRenderPass<RenderContext>::PassOutputType BilateralUpsampleRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template BilateralUpsampleRenderPass<ParallelRenderContext>::PassOutputType BilateralUpsampleRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template BilateralUpsampleRenderPass<VulkanRenderContext>::PassOutputType BilateralUpsampleRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
