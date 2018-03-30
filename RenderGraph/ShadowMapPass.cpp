#include "ShadowMapPass.h"
#include "DepthPass.h"


typename ShadowMapRenderPass::PassOutputType ShadowMapRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
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
		{
			Builder.BuildRenderPass("ShadowMap_DepthRenderPass", DepthRenderPass::Build),
			Builder.RenameOutputToOutput<RDAG::DepthTarget, RDAG::ShadowMapTextureArray>(0, i)
		}(Output);
	}
	return Output;
}