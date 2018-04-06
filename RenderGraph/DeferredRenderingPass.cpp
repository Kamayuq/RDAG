#include "DeferredRenderingPass.h"
#include "DepthPass.h"
#include "GbufferPass.h"
#include "ForwardPass.h"
#include "ShadowMapPass.h"
#include "VelocityPass.h"
#include "AmbientOcclusion.h"
#include "DeferredLightingPass.h"
#include "TransparencyPass.h"
#include "TemporalAA.h"

typename DeferredRendererPass::PassOutputType DeferredRendererPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	using DeferredResult = ResourceTable
	<
		InputTable<RDAG::TransparencyInput>,
		OutputTable<>
	>;
	using PostprocessingData = ResourceTable
	<
		InputTable<RDAG::SceneViewInfo, RDAG::DepthTexture, RDAG::VelocityVectors, RDAG::TransparencyResult>,
		OutputTable<>
	>;
	return Seq
	{
		Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass::Build),
		Extract<DeferredResult>(Seq
		{
			Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass::Build),
			Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass::Build),
			Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass::Build),
			Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass::Build),
			Builder.RenameOutputToInput<RDAG::LightingUAV, RDAG::TransparencyInput>(),
		}),
		Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass::Build),
		Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass::Build),
		Select<PostprocessingData>(Seq
		{
			Scope(Seq
			{
				Builder.RenameInputToInput<RDAG::TransparencyResult, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
				Builder.RenameOutputToInput<RDAG::TemporalAAOutput, RDAG::TransparencyResult>()
			}),
			Builder.RenameInputToInput<RDAG::TransparencyResult, RDAG::PostProcessingInput>(),
			Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass::Build)
		})
	}(Input);
}
