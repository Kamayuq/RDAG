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


auto RunGbufferPasses(const RenderPassBuilder& Builder)
{
	return Seq
	{
		Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass::Build),
		Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass::Build)
	};
}

auto RunShadowAndLightingPasses(const RenderPassBuilder& Builder)
{
	return Seq
	{
		Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass::Build),
		Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass::Build),
		Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass::Build)
	};
}

auto RunTransparencyPasses(const RenderPassBuilder& Builder)
{
	return Seq
	{
		Builder.RenameOutputToInput<RDAG::LightingUAV, RDAG::TransparencyInput>(),
		Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass::Build)
	};
}

auto RunPostprocessingPasses(const RenderPassBuilder& Builder)
{
	return Seq
	{
		Builder.RenameOutputToInput<RDAG::TransparencyResult, RDAG::TemporalAAInput>(),
		Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass::Build),
		Builder.RenameOutputToInput<RDAG::TemporalAAOutput, RDAG::PostProcessingInput>(),
		Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass::Build)
	};
}

typename DeferredRendererPass::PassOutputType DeferredRendererPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
#if ASYNC_VELOCITY
	Promise<typename VelocityRenderPass::PassOutputType> VelocityPassPromise;
#endif
	return Seq
	{
		RunGbufferPasses(Builder),
#if ASYNC_VELOCITY
		Builder.BuildAsyncRenderPass("VelocityRenderPass", VelocityRenderPass::Build, VelocityPassPromise),
#else
		Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass::Build),
#endif
		RunShadowAndLightingPasses(Builder),
		RunTransparencyPasses(Builder),
#if ASYNC_VELOCITY
		Builder.SynchronizeAsyncRenderPass(VelocityPassPromise),
#endif
		RunPostprocessingPasses(Builder)
	}(Input);
}