#include "ShadowMapPass.h"
#include "DepthPass.h"

template<typename RenderContextType>
typename ShadowMapRenderPass<RenderContextType>::PassOutputType ShadowMapRenderPass<RenderContextType>::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{	
	PassOutputType Output = Builder.CreateOutputResource<RDAG::ShadowMapTextureArray>({})(Input);

	RDAG::SceneViewInfo ShadowViewInfo = Input.GetInputHandle<RDAG::SceneViewInfo>();
	ShadowViewInfo.SceneWidth = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.SceneHeight = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.DepthFormat = ShadowViewInfo.ShadowFormat;

	check(ShadowViewInfo.ShadowCascades <= RDAG::ShadowMapTextureArray::ResourceCount);
	for (U32 i = 0; i < ShadowViewInfo.ShadowCascades; i++)
	{
		Output = Seq
		(
			Builder.BuildRenderPass("ShadowMap_DepthRenderPass", DepthRenderPass<RenderContextType>::Build),
			Builder.MoveOutputTableEntry<RDAG::DepthTarget, RDAG::ShadowMapTextureArray>(0, i)
		)(Output);
	}
	return Output;
}
template ShadowMapRenderPass<RenderContext>::PassOutputType ShadowMapRenderPass<RenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ShadowMapRenderPass<ParallelRenderContext>::PassOutputType ShadowMapRenderPass<ParallelRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
template ShadowMapRenderPass<VulkanRenderContext>::PassOutputType ShadowMapRenderPass<VulkanRenderContext>::Build(const RenderPassBuilder&, const PassInputType&);
