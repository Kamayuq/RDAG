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

	return Seq
	{
		Builder.CreateResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction<RDAG::PostProcessingResult>("ToneMappingAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("ToneMappingAction");
		})
	}(Input);
}

typename PostProcessingPass::PassOutputType PostProcessingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	return Seq
	{
		Scope(Seq
		{
			Builder.RenameEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass<RDAG::DownsamplePyramid<16>>("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass<16>::Build),
			Builder.RenameEntry<RDAG::DownsamplePyramid<16>, RDAG::DepthOfFieldInput>(4, 0),
			Builder.BuildRenderPass<RDAG::DepthOfFieldOutput>("DepthOfFieldRenderPass", DepthOfFieldPass::Build),
			Builder.RenameEntry<RDAG::DepthOfFieldOutput, RDAG::PostProcessingInput>()
		}),
		Builder.BuildRenderPass<RDAG::PostProcessingResult>("ToneMappingPass", ToneMappingPass::Build)
	}(Input);
}
