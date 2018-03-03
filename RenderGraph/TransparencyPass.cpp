#include "TransparencyPass.h"
#include "DownSamplePass.h"
#include "BilateralUpsample.h"
#include "SimpleBlendPass.h"

template<typename RenderContextType>
typename HalfResTransparencyRenderPass<RenderContextType>::PassOutputType HalfResTransparencyRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto TransparencyInfo = Input.GetInputDescriptor<RDAG::TransparencyInput>();
	Texture2d::Descriptor HalfResTransparencyDescriptor;
	HalfResTransparencyDescriptor.Name = "HalfResTransparencyRenderTarget";
	HalfResTransparencyDescriptor.Format = TransparencyInfo.Format;
	HalfResTransparencyDescriptor.Height = TransparencyInfo.Height >> 1;
	HalfResTransparencyDescriptor.Width = TransparencyInfo.Width >> 1;

	return Seq
	(
		Seq
		(
			Builder.CreateOutputResource<RDAG::HalfResTransparencyResult>({ HalfResTransparencyDescriptor }),
			Builder.MoveInputTableEntry<RDAG::DepthTarget, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass("HalfResTransparency_DownsampleRenderPass", DownsampleRenderPass<RenderContextType>::Build),
			Builder.MoveOutputTableEntry<RDAG::DownsampleResult, RDAG::DepthTarget>(),
			Builder.MoveOutputTableEntry<RDAG::HalfResTransparencyResult, RDAG::ForwardRender>(),
			Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass<RenderContextType>::Build),
			Builder.MoveOutputToInputTableEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
			Builder.MoveOutputToInputTableEntry<RDAG::ForwardRender, RDAG::HalfResInput>()
		),
		Builder.MoveInputToOutputTableEntry<RDAG::DepthTarget, RDAG::DepthTarget>(), //restore original depth from before the forward pass
		Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass<RenderContextType>::Build),
		Builder.MoveOutputTableEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
	)(Input);
}
template HalfResTransparencyRenderPass<RenderContext>::PassOutputType HalfResTransparencyRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template HalfResTransparencyRenderPass<ParallelRenderContext>::PassOutputType HalfResTransparencyRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template HalfResTransparencyRenderPass<VulkanRenderContext>::PassOutputType HalfResTransparencyRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);

template<typename RenderContextType>
typename TransparencyRenderPass<RenderContextType>::PassOutputType TransparencyRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	PassOutputType Output = Builder.MoveInputToOutputTableEntry<RDAG::TransparencyInput, RDAG::TransparencyResult>()(Input);

	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();

	if (ViewInfo.TransparencyEnabled)
	{
		Output = Seq
		(
			Builder.MoveOutputTableEntry<RDAG::TransparencyResult, RDAG::ForwardRender>(),
			Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass<RenderContextType>::Build),
			Builder.MoveOutputTableEntry<RDAG::ForwardRender, RDAG::TransparencyResult>()
		)(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		(
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass<RenderContextType>::Build),
			Builder.MoveOutputTableEntry<RDAG::TransparencyResult, RDAG::BlendSource>(0, 0),
			Builder.MoveOutputTableEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass<RenderContextType>::Build),
			Builder.MoveOutputTableEntry<RDAG::BlendDest, RDAG::TransparencyResult>()
		)(Output);
	}
	return Output;
}
template TransparencyRenderPass<RenderContext>::PassOutputType TransparencyRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template TransparencyRenderPass<ParallelRenderContext>::PassOutputType TransparencyRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template TransparencyRenderPass<VulkanRenderContext>::PassOutputType TransparencyRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
