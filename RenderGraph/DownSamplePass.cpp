#include "DownSamplePass.h"


typename DownsampleRenderPass::DownsampleRenderResult DownsampleRenderPass::Build(const RenderPassBuilder& Builder, const DownsampleRenderInput& Input)
{
	const Texture2d::Descriptor& DownSampleInfo = Input.GetDescriptor<RDAG::DownsampleInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	using DownsampleRenderAction = decltype(std::declval<DownsampleRenderInput>().Union(std::declval<DownsampleRenderResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::DownsampleResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const DownsampleRenderAction&)
		{
			Ctx.Draw("DownsampleRenderAction");
		})
	}(Input);
}

typename DownsampleDepthRenderPass::DownsampleDepthResult DownsampleDepthRenderPass::Build(const RenderPassBuilder& Builder, const DownsampleDepthInput& Input)
{
	const Texture2d::Descriptor& DownSampleInfo = Input.GetDescriptor<RDAG::DownsampleDepthInput>();
	Texture2d::Descriptor DownsampleDescriptor;
	DownsampleDescriptor.Name = "DownsampleDepthRenderTarget";
	DownsampleDescriptor.Format = DownSampleInfo.Format;
	DownsampleDescriptor.Height = DownSampleInfo.Height >> 1;
	DownsampleDescriptor.Width = DownSampleInfo.Width >> 1;

	using DownsampleDepthAction = decltype(std::declval<DownsampleDepthInput>().Union(std::declval<DownsampleDepthResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::DownsampleDepthResult>({ DownsampleDescriptor }),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const DownsampleDepthAction&)
	{
		Ctx.Draw("DownsampleDepthRenderAction");
	})
	}(Input);
}

typename PyramidDownSampleRenderPass::PyramidDownSampleRenderResult PyramidDownSampleRenderPass::Build(const RenderPassBuilder& Builder, const PyramidDownSampleRenderInput& Input)
{
	PyramidDownSampleRenderResult Output = Builder.RenameEntry<RDAG::DownsampleInput, RDAG::DownsamplePyramid>(0, 0)(Input);

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