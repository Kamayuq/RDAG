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
	using DeferredResult = ResourceTable<RDAG::TransparencyInput>;
	using PostprocessingData = ResourceTable<RDAG::SceneViewInfo, RDAG::DepthTarget, RDAG::VelocityVectors, RDAG::TransparencyResult>;
	return Seq
	{
		Builder.BuildRenderPass<RDAG::DepthTarget>("DepthRenderPass", DepthRenderPass::Build),
		Extract<DeferredResult>(Seq
		{
			Builder.BuildRenderPass<RDAG::DepthTarget, RDAG::GbufferTarget>("GbufferRenderPass", GbufferRenderPass::Build),
			Builder.BuildRenderPass<RDAG::AmbientOcclusionTexture>("AmbientOcclusionPass", AmbientOcclusionPass::Build),
			Builder.BuildRenderPass<RDAG::ShadowMapTextureArray>("ShadowMapRenderPass", ShadowMapRenderPass::Build),
			Builder.BuildRenderPass<RDAG::LightingUAV>("DeferredLightingPass", DeferredLightingPass::Build),
			Builder.RenameEntry<RDAG::LightingUAV, RDAG::TransparencyInput>(),
		}),
		Builder.BuildRenderPass<RDAG::DepthTarget, RDAG::TransparencyResult>("TransparencyRenderPass", TransparencyRenderPass::Build),
		Builder.BuildRenderPass<RDAG::VelocityVectors>("VelocityRenderPass", VelocityRenderPass::Build),
		Select<PostprocessingData>(Seq
		{
			Scope(Seq
			{
				Builder.RenameEntry<RDAG::TransparencyResult, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass<RDAG::TemporalAAOutput>("TemporalAARenderPass", TemporalAARenderPass::Build),
				Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::TransparencyResult>()
			}),
			Builder.RenameEntry<RDAG::TransparencyResult, RDAG::PostProcessingInput>(),
			Builder.BuildRenderPass<RDAG::PostProcessingResult>("PostProcessingPass", PostProcessingPass::Build)
		})
	}(Input);
}
