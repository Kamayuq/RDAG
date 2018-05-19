#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"
#include "DepthOfField.h"


typename ToneMappingPass::PassOutputType ToneMappingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto PPfxInfo = Input.GetDescriptor<RDAG::PostProcessingInput>();
	ExternalTexture2dDescriptor ResultDescriptor;
	ResultDescriptor.Index = 0x10;
	ResultDescriptor.Name = "PostProcessingResult";
	ResultDescriptor.Format = ERenderResourceFormat::ARGB8U;
	ResultDescriptor.Height = PPfxInfo.Height;
	ResultDescriptor.Width = PPfxInfo.Width;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.CreateResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction("ToneMappingAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("ToneMappingAction");
		})
	}(Input);
}

typename PostProcessingPass::PassOutputType PostProcessingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
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
