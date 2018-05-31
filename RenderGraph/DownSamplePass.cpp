#include "DownSamplePass.h"


typename DownsampleRenderPass::PassOutputType DownsampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const Texture2d::Descriptor& DownSampleInfo = Input.GetDescriptor<RDAG::DownsampleInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.CreateResource<RDAG::DownsampleResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("DownsampleRenderAction");
		})
	}(Input);
}

typename DownsampleDepthRenderPass::PassOutputType DownsampleDepthRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const Texture2d::Descriptor& DownSampleInfo = Input.GetDescriptor<RDAG::DownsampleDepthInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleDepthRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.CreateResource<RDAG::DownsampleDepthResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const PassActionType&)
	{
		Ctx.Draw("DownsampleDepthRenderAction");
	})
	}(Input);
}

typename PyramidDownSampleRenderPass::PassOutputType PyramidDownSampleRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	PassOutputType Output = Builder.RenameEntry<RDAG::DownsampleInput, RDAG::DownsamplePyramid>(0, 0)(Input);

	for (U32 i = 1; true; i++)
	{
		const Texture2d::Descriptor& DownSampleInfo = Output.template GetDescriptor<RDAG::DownsamplePyramid>(i - 1);
		if (((DownSampleInfo.Width >> 1) == 0) || ((DownSampleInfo.Height >> 1) == 0))
			break;

		Output = Seq
		{
			Builder.RenameEntry<RDAG::DownsamplePyramid, RDAG::DownsampleInput>(i-1, 0),
			Builder.BuildRenderPass("DownsampleRenderPass", DownsampleRenderPass::Build),
			Builder.RenameEntry<RDAG::DownsampleResult, RDAG::DownsamplePyramid>(0, i)
		}(Output);
	}
	return Output;
}