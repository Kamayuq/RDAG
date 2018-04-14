#include "ShadowMapPass.h"
#include "DepthPass.h"


typename ShadowMapRenderPass::PassOutputType ShadowMapRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{	
	auto Output = Builder.CreateResource<RDAG::ShadowMapTextureArray>()(Input);

	RDAG::SceneViewInfo ShadowViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	ShadowViewInfo.SceneWidth = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.SceneHeight = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.DepthFormat = ShadowViewInfo.ShadowFormat;

	for (U32 i = 0; i < ShadowViewInfo.ShadowCascades; i++)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("ShadowMap_DepthRenderPass", DepthRenderPass::Build),
			Builder.RenameEntry<RDAG::DepthTarget, RDAG::ShadowMapTextureArray>(0, i)
		}(Output);
	}
	return Output;
}