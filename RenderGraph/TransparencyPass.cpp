#include "TransparencyPass.h"
#include "DownSamplePass.h"
#include "BilateralUpsample.h"
#include "SimpleBlendPass.h"


typename HalfResTransparencyRenderPass::PassOutputType HalfResTransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
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
			Builder.BuildRenderPass("HalfResTransparency_DownsampleRenderPass", DownsampleRenderPass::Build),
			Builder.MoveOutputTableEntry<RDAG::DownsampleResult, RDAG::DepthTarget>(),
			Builder.MoveOutputTableEntry<RDAG::HalfResTransparencyResult, RDAG::ForwardRenderTarget>(),
			Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.MoveOutputToInputTableEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
			Builder.MoveOutputToInputTableEntry<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
		),
		Builder.MoveInputToOutputTableEntry<RDAG::DepthTarget, RDAG::DepthTarget>(), //restore original depth from before the forward pass
		Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
		Builder.MoveOutputTableEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
	)(Input);
}

typename TransparencyRenderPass::PassOutputType TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	PassOutputType Output = Builder.MoveInputToOutputTableEntry<RDAG::TransparencyInput, RDAG::TransparencyResult>()(Input);

	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();

	if (ViewInfo.TransparencyEnabled)
	{
		Output = Seq
		(
			Builder.MoveOutputTableEntry<RDAG::TransparencyResult, RDAG::ForwardRenderTarget>(),
			Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.MoveOutputTableEntry<RDAG::ForwardRenderTarget, RDAG::TransparencyResult>()
		)(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		(
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.MoveOutputTableEntry<RDAG::TransparencyResult, RDAG::BlendSource>(0, 0),
			Builder.MoveOutputTableEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build),
			Builder.MoveOutputTableEntry<RDAG::BlendDest, RDAG::TransparencyResult>()
		)(Output);
	}
	return Output;
}