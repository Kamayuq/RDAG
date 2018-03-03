#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"

template<typename RenderContextType>
typename ToneMappingPass<RenderContextType>::PassOutputType ToneMappingPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto PPfxInfo = Input.GetInputDescriptor<RDAG::PostProcessingInput>();
	ExternalTexture2dResourceHandle::Descriptor ResultDescriptor;
	ResultDescriptor.Index = 0x10;
	ResultDescriptor.Name = "PostProcessingResult";
	ResultDescriptor.Format = ERenderResourceFormat::ARGB8U;
	ResultDescriptor.Height = PPfxInfo.Height;
	ResultDescriptor.Width = PPfxInfo.Width;

	return Seq
	(
		Builder.CreateOutputResource<RDAG::PostProcessingResult>({ ResultDescriptor }),
		Builder.QueueRenderAction("ToneMappingAction", [](RenderContextType&, const PassOutputType&)
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
template ToneMappingPass<RenderContext>::PassOutputType ToneMappingPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ToneMappingPass<ParallelRenderContext>::PassOutputType ToneMappingPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ToneMappingPass<VulkanRenderContext>::PassOutputType ToneMappingPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);

template<typename RenderContextType>
typename PostProcessingPass<RenderContextType>::PassOutputType PostProcessingPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	return Seq
	(
		Builder.MoveInputTableEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
		Builder.BuildRenderPass("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass<16, RenderContextType>::Build),
		Builder.MoveOutputToInputTableEntry<RDAG::DownsamplePyramid<16>, RDAG::PostProcessingInput>(4, 0),
		Builder.BuildRenderPass("ToneMappingPass", ToneMappingPass<RenderContextType>::Build)
	)(Input);
}
template PostProcessingPass<RenderContext>::PassOutputType PostProcessingPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template PostProcessingPass<ParallelRenderContext>::PassOutputType PostProcessingPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template PostProcessingPass<VulkanRenderContext>::PassOutputType PostProcessingPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
