#include "DownSamplePass.h"
#include "CopyTexturePass.h"


typename DownsampleRenderPass::DownsampleRenderResult DownsampleRenderPass::Build(const RenderPassBuilder& Builder, const DownsampleRenderInput& Input)
{
	const Texture2d::Descriptor& DownSampleInInfo = Input.GetDescriptor<RDAG::DownsampleInput>();
	const Texture2d::Descriptor& DownSampleOutInfo = Input.GetDescriptor<RDAG::DownsampleResult>();
	check(DownSampleInInfo.Height >> 1 == DownSampleOutInfo.Height);
	check(DownSampleInInfo.Width >> 1 == DownSampleOutInfo.Width);
	check(DownSampleInInfo.Format == DownSampleOutInfo.Format);

	using DownsampleRenderAction = decltype(std::declval<DownsampleRenderInput>().Union(std::declval<DownsampleRenderResult>()));
	return Seq
	{
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
		Builder.CreateResource<RDAG::DownsampleDepthResult>( DownsampleDescriptor ),
		Builder.QueueRenderAction("DownsampleRenderAction", [](RenderContext& Ctx, const DownsampleDepthAction&)
	{
		Ctx.Draw("DownsampleDepthRenderAction");
	})
	}(Input);
}

typename PyramidDownSampleRenderPass::PyramidDownSampleRenderResult PyramidDownSampleRenderPass::Build(const RenderPassBuilder& Builder, const PyramidDownSampleRenderInput& Input)
{
	Texture2d::Descriptor PyramidDescriptor = Input.GetDescriptor<RDAG::DownsampleInput>();
	PyramidDescriptor.Name = "PyramidTexture";
	PyramidDescriptor.ComputeFullMipChain();

	PyramidDownSampleRenderResult Output = Seq
	{
		Builder.CreateResource<RDAG::DownsamplePyramid>(PyramidDescriptor),
		Builder.AssignEntry<RDAG::DownsamplePyramid, RDAG::CopyDestination>(0),
		Builder.AssignEntry<RDAG::DownsampleInput, RDAG::CopySource>(),
		Builder.BuildRenderPass("CopyInitialPyramidSlice", CopyTexturePass::Build),
		Builder.AssignEntry<RDAG::CopyDestination, RDAG::DownsamplePyramid>()
	}(Input);

	for (U32 i = 1; true; i++)
	{
		if (((PyramidDescriptor.Width >>= 1) == 0) || ((PyramidDescriptor.Height >>= 1) == 0))
			break;

		Output = Seq
		{
			Builder.AssignEntry<RDAG::DownsamplePyramid, RDAG::DownsampleInput>(i-1),
			Builder.AssignEntry<RDAG::DownsamplePyramid, RDAG::DownsampleResult>(i),
			Builder.BuildRenderPass("DownsampleRenderPass", DownsampleRenderPass::Build),
			Builder.AssignEntry<RDAG::DownsampleResult, RDAG::DownsamplePyramid>()
		}(Output);
	}
	return Output;
}