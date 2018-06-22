#include "PostprocessingPass.h"
#include "DownSamplePass.h"
#include "TemporalAA.h"
#include "DepthOfField.h"


typename ToneMappingPass::ToneMappingResult ToneMappingPass::Build(const RenderPassBuilder& Builder, const ToneMappingInput& Input)
{
	Texture2d::Descriptor ResultDescriptor = Input.GetDescriptor<RDAG::PostProcessingInput>();
	ResultDescriptor.Name = "PostProcessingResult";
	ResultDescriptor.Format = ERenderResourceFormat::ARGB8U;

	using ToneMappingAction = decltype(std::declval<ToneMappingInput>().Union(std::declval<ToneMappingResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::PostProcessingResult>( ResultDescriptor ),
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
			Builder.AssignEntry<RDAG::PostProcessingInput, RDAG::DownsampleInput>(),
			Builder.BuildRenderPass("PyramidDownSampleRenderPass", PyramidDownSampleRenderPass::Build),
			Builder.AssignEntry<RDAG::DownsamplePyramid, RDAG::SceneColorTexture>(4),
			Builder.BuildRenderPass("DepthOfFieldRenderPass", DepthOfFieldPass::Build, ViewInfo)
		}),
		Builder.BuildRenderPass("ToneMappingPass", ToneMappingPass::Build)
	}(Input);
}
