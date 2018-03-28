#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"
#include "DepthOfField.h"


typename ToneMappingPass::PassOutputType ToneMappingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto PPfxInfo = Input.GetInputDescriptor<RDAG::PostProcessingInput>();
	ExternalTexture2dDescriptor ResultDescriptor;
	ResultDescriptor.Index = 0x10;
	ResultDescriptor.Name = "PostProcessingResult";
	ResultDescriptor.Format = ERenderResourceFormat::ARGB8U;
	ResultDescriptor.Height = PPfxInfo.Height;
	ResultDescriptor.Width = PPfxInfo.Width;

	return Seq
	{
		Builder.CreateOutputResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction("ToneMappingAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("ToneMappingAction");
		})
	}(Input);
}

typename PostProcessingPass::PassOutputType PostProcessingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	return Seq
	{
		Builder.MoveInputTableEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
		Builder.BuildRenderPass("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass<16>::Build),
		Builder.MoveOutputToInputTableEntry<RDAG::DownsamplePyramid<16>, RDAG::DepthOfFieldInput>(4, 0),
		Builder.BuildRenderPass("DepthOfFieldRenderPass", DepthOfFieldPass::Build),
		Builder.MoveOutputToInputTableEntry<RDAG::DepthOfFieldOutput, RDAG::PostProcessingInput>(),
		Builder.BuildRenderPass("ToneMappingPass", ToneMappingPass::Build)
	}(Input);
}