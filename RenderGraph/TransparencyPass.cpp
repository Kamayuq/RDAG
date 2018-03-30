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
	{
		Seq
		{
			Builder.CreateOutputResource<RDAG::HalfResTransparencyResult>({ HalfResTransparencyDescriptor }),
			Builder.RenameInputToInput<RDAG::DepthTarget, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass("HalfResTransparency_DownsampleRenderPass", DownsampleRenderPass::Build),
			Builder.RenameOutputToOutput<RDAG::DownsampleResult, RDAG::DepthTarget>(),
			Builder.RenameOutputToOutput<RDAG::HalfResTransparencyResult, RDAG::ForwardRenderTarget>(),
			Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.RenameOutputToInput<RDAG::DepthTarget, RDAG::HalfResDepth>(),
			Builder.RenameOutputToInput<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
		},
		Builder.RenameInputToOutput<RDAG::DepthTarget, RDAG::DepthTarget>(), //restore original depth from before the forward pass
		Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
		Builder.RenameOutputToOutput<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
	}(Input);
}

typename TransparencyRenderPass::PassOutputType TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	PassOutputType Output = Builder.RenameInputToOutput<RDAG::TransparencyInput, RDAG::TransparencyResult>()(Input);

	const RDAG::SceneViewInfo& ViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	if (ViewInfo.TransparencyEnabled)
	{
		Output = Seq
		{
			Builder.RenameOutputToOutput<RDAG::TransparencyResult, RDAG::ForwardRenderTarget>(),
			Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.RenameOutputToOutput<RDAG::ForwardRenderTarget, RDAG::TransparencyResult>()
		}(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.RenameOutputToOutput<RDAG::TransparencyResult, RDAG::BlendSource>(0, 0),
			Builder.RenameOutputToOutput<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build),
			Builder.RenameOutputToOutput<RDAG::BlendDest, RDAG::TransparencyResult>()
		}(Output);
	}
	return Output;
}