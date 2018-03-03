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

template<typename RenderContextType>
auto RunGbufferPasses(const RenderPassBuilder& Builder)
{
	return Seq
	(
		Builder.BuildRenderPass("DepthRenderPass", DepthRenderPass<RenderContextType>::Build),
		Builder.BuildRenderPass("GbufferRenderPass", GbufferRenderPass<RenderContextType>::Build)
	);
}

template<typename RenderContextType>
auto RunShadowAndLightingPasses(const RenderPassBuilder& Builder)
{
	return Seq
	(
		Builder.BuildRenderPass("AmbientOcclusionPass", AmbientOcclusionPass<RenderContextType>::Build),
		Builder.BuildRenderPass("ShadowMapRenderPass", ShadowMapRenderPass<RenderContextType>::Build),
		Builder.BuildRenderPass("DeferredLightingPass", DeferredLightingPass<RenderContextType>::Build)
	);
}

template<typename RenderContextType>
auto RunTransparencyPasses(const RenderPassBuilder& Builder)
{
	return Seq
	(
		Builder.MoveOutputToInputTableEntry<RDAG::LightingResult, RDAG::TransparencyInput>(),
		Builder.BuildRenderPass("TransparencyRenderPass", TransparencyRenderPass<RenderContextType>::Build)
	);
}

template<typename RenderContextType>
auto RunPostprocessingPasses(const RenderPassBuilder& Builder)
{
	return Seq
	(
		Builder.MoveOutputToInputTableEntry<RDAG::TransparencyResult, RDAG::TemporalAAInput>(),
		Builder.BuildRenderPass("TemporalAARenderPass", TemporalAARenderPass<RenderContextType>::Build),
		Builder.MoveOutputToInputTableEntry<RDAG::TemporalAAOutput, RDAG::PostProcessingInput>(),
		Builder.BuildRenderPass("PostProcessingPass", PostProcessingPass<RenderContextType>::Build)
	);
}

template<typename RenderContextType>
typename DeferredRendererPass<RenderContextType>::PassOutputType DeferredRendererPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
#if ASYNC_VELOCITY
	Promise<typename VelocityRenderPass<RenderContextType>::PassOutputType> VelocityPassPromise;
#endif
	return Seq
	(
		RunGbufferPasses<RenderContextType>(Builder),
#if ASYNC_VELOCITY
		Builder.BuildAsyncRenderPass("VelocityRenderPass", VelocityRenderPass<RenderContextType>::Build, VelocityPassPromise),
#else
		Builder.BuildRenderPass("VelocityRenderPass", VelocityRenderPass<RenderContextType>::Build),
#endif
		RunShadowAndLightingPasses<RenderContextType>(Builder),
		RunTransparencyPasses<RenderContextType>(Builder),
#if ASYNC_VELOCITY
		Builder.SynchronizeAsyncRenderPass(VelocityPassPromise),
#endif
		RunPostprocessingPasses<RenderContextType>(Builder)
	)(Input);
}
template DeferredRendererPass<RenderContext>::PassOutputType DeferredRendererPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DeferredRendererPass<ParallelRenderContext>::PassOutputType DeferredRendererPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template DeferredRendererPass<VulkanRenderContext>::PassOutputType DeferredRendererPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
