#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"


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
	(
		Builder.CreateOutputResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction("ToneMappingAction", [](RenderContext&, const PassOutputType&)
		{
			//auto ToneMapInput = FDataSet::GetStaticResource<RDAG::FPostProcessingInput>(RndCtx, Self->PassData);
			//(void)ToneMapInput;
			//auto ToneMapOutput = FDataSet::GetMutableResource<RDAG::FPostProcessingResult>(RndCtx, Self->PassData);
			//(void)ToneMapOutput;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindStatic(ToneMapInput);
			//RndCtx.BindMutable(ToneMapOutput);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}

typename PostProcessingPass::PassOutputType PostProcessingPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	return Seq
	(
		Builder.MoveInputTableEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
		Builder.BuildRenderPass("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass<16>::Build),
		Builder.MoveOutputToInputTableEntry<RDAG::DownsamplePyramid<16>, RDAG::PostProcessingInput>(4, 0),
		Builder.BuildRenderPass("ToneMappingPass", ToneMappingPass::Build)
	)(Input);
}