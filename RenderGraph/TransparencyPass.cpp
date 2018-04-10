#include "TransparencyPass.h"
#include "DownSamplePass.h"
#include "BilateralUpsample.h"
#include "SimpleBlendPass.h"


typename HalfResTransparencyRenderPass::PassOutputType HalfResTransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto TransparencyInfo = Input.GetDescriptor<RDAG::TransparencyInput>();
	Texture2d::Descriptor HalfResTransparencyDescriptor;
	HalfResTransparencyDescriptor.Name = "HalfResTransparencyRenderTarget";
	HalfResTransparencyDescriptor.Format = TransparencyInfo.Format;
	HalfResTransparencyDescriptor.Height = TransparencyInfo.Height >> 1;
	HalfResTransparencyDescriptor.Width = TransparencyInfo.Width >> 1;

	using SelectionType = ResourceTable<RDAG::HalfResInput, RDAG::HalfResDepth>;
	return Seq
	{
		Extract<SelectionType>(Seq
		{
			Scope(Seq
			{
				Builder.RenameEntry<RDAG::DepthTarget, RDAG::DownsampleInput>(),
				Builder.BuildRenderPass("HalfResTransparency_DownsampleRenderPass", DownsampleRenderPass::Build),
				Builder.RenameEntry<RDAG::DownsampleResult, RDAG::DepthTarget>()
			}),
			Builder.RenameEntry<RDAG::DepthTarget, RDAG::DepthTarget>(),
			Builder.CreateResource<RDAG::ForwardRenderTarget>({ HalfResTransparencyDescriptor }, ESortOrder::BackToFront),
			Builder.BuildRenderPass("HalfResTransparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.RenameEntry<RDAG::DepthTarget, RDAG::HalfResDepth>(),
			Builder.RenameEntry<RDAG::ForwardRenderTarget, RDAG::HalfResInput>()
		}),
		//Builder.RenameInputToOutput<RDAG::DepthTarget, RDAG::DepthTarget>(), //restore original depth from before the forward pass
		Builder.BuildRenderPass("HalfResTransparency_BilateralUpsampleRenderPass", BilateralUpsampleRenderPass::Build),
		Builder.RenameEntry<RDAG::UpsampleResult, RDAG::HalfResTransparencyResult>()
	}(Input);
}

typename TransparencyRenderPass::PassOutputType TransparencyRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto Output = Builder.RenameEntry<RDAG::TransparencyInput, RDAG::TransparencyResult>()(Input);

	const RDAG::SceneViewInfo& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	if (ViewInfo.TransparencyEnabled)
	{
		Output = Seq
		{
			Builder.RenameEntry<RDAG::TransparencyResult, RDAG::ForwardRenderTarget>(),
			Builder.BuildRenderPass("Transparency_ForwardRenderPass", ForwardRenderPass::Build),
			Builder.RenameEntry<RDAG::ForwardRenderTarget, RDAG::TransparencyResult>()
		}(Output);
	}

	if (ViewInfo.TransparencySeperateEnabled)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("HalfResTransparencyRenderPass", HalfResTransparencyRenderPass::Build),
			Builder.RenameEntry<RDAG::TransparencyResult, RDAG::BlendSource>(0, 0),
			Builder.RenameEntry<RDAG::HalfResTransparencyResult, RDAG::BlendSource>(0, 1),
			Builder.BuildRenderPass("Transparency_FSimpleBlendPass", SimpleBlendPass::Build),
			Builder.RenameEntry<RDAG::BlendDest, RDAG::TransparencyResult>()
		}(Output);
	}
	return Output;
}
