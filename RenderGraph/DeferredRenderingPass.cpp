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
	using LightingResult = ResourceTable<InputTable<RDAG::SceneViewInfo>, OutputTable<RDAG::DepthTexture, RDAG::LightingUAV>>;
	return Seq
	{
		Extract<LightingResult>(Seq
		{
			Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass::Build),
			Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass::Build),
			Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass::Build),
			Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass::Build),
			Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass::Build)
		}),
		Select<LightingResult>(Seq
		{
			Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass::Build),
			Builder.RenameOutputToInput<RDAG::LightingUAV, RDAG::TransparencyInput>(),
			Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass::Build),
			Scope(Seq
			{
				Builder.RenameOutputToInput<RDAG::TransparencyResult, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
				Builder.RenameOutputToInput<RDAG::TemporalAAOutput, RDAG::TransparencyResult>()
			}),
			Builder.RenameOutputToInput<RDAG::TransparencyResult, RDAG::PostProcessingInput>(),
			Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass::Build)
		})
	}(Input);
}
