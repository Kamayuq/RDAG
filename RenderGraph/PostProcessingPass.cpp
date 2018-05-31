#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"
#include "DepthOfField.h"


typename ToneMappingPass::ToneMappingResult ToneMappingPass::Build(const RenderPassBuilder& Builder, const ToneMappingInput& Input)
{
	const Texture2d::Descriptor& PPfxInfo = Input.GetDescriptor<RDAG::PostProcessingInput>();
	ExternalTexture2dDescriptor ResultDescriptor;
	ResultDescriptor.Index = 0x10;
	ResultDescriptor.Name = "PostProcessingResult";
	ResultDescriptor.Format = ERenderResourceFormat::ARGB8U;
	ResultDescriptor.Height = PPfxInfo.Height;
	ResultDescriptor.Width = PPfxInfo.Width;

	using ToneMappingAction = decltype(std::declval<ToneMappingInput>().Union(std::declval<ToneMappingResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction("ToneMappingAction", [](RenderContext& Ctx, const ToneMappingAction&)
		{
			Ctx.Draw("ToneMappingAction");
		})
	}(Input);
}

typename PostProcessingPass::PostProcessingPassResult PostProcessingPass::Build(const RenderPassBuilder& Builder, const PostProcessingPassInput& Input, const SceneViewInfo& ViewInfo)
{
	return Seq
	{
		Scope(Seq
		{
			Builder.RenameEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass::Build),
			Builder.RenameEntry<RDAG::DownsamplePyramid, RDAG::SceneColorTexture>(4, 0),
			Builder.BuildRenderPass("DepthOfFieldRenderPass", DepthOfFieldPass::Build, ViewInfo)
		}),
		Builder.BuildRenderPass("ToneMappingPass", ToneMappingPass::Build)
	}(Input);
}
