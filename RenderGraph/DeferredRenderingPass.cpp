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

typename DeferredRendererPass::PassOutputType DeferredRendererPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
{
	return Seq
	{
		Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass::Build, ViewInfo),
		Extract<RDAG::SceneColorTexture>(Seq
		{
			Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass::Build),
			Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass::Build, ViewInfo),
			Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass::Build, ViewInfo),
			Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass::Build),
		}),
		Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass::Build, ViewInfo),
		Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass::Build),
		Select<RDAG::VelocityVectors, RDAG::SceneColorTexture, RDAG::DepthTarget>(Seq
		{
			Scope(Seq
			{
				Builder.RenameEntry<RDAG::SceneColorTexture, RDAG::TemporalAAInput>(),
				Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build, ViewInfo),
				Builder.RenameEntry<RDAG::TemporalAAOutput, RDAG::SceneColorTexture>()
			}),
			Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass::Build, ViewInfo)
		})
	}(Input);
}
