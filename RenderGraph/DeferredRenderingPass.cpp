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
	return Seq
	{
		Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass::Build),
		Extract<RDAG::SceneColorTexture>(Seq
		{
			Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass::Build),
			Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass::Build),
			Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass::Build),
			Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass::Build),
		}),
		Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass::Build),
		Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass::Build),
		Select<RDAG::VelocityVectors, RDAG::SceneColorTexture, RDAG::DepthTarget, RDAG::SceneViewInfo>(Seq
		{
			Scope(Seq
			{
				Builder.RenameEntry<RDAG::SceneColorTexture, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
				Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::SceneColorTexture>()
			}),
			Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass::Build)
		})
	}(Input);
}
