#include "ShadowMapPass.h"
#include "DepthPass.h"


typename ShadowMapRenderPass::ShadowMapRenderResult ShadowMapRenderPass::Build(const RenderPassBuilder& Builder, const ShadowMapRenderInput& Input, const SceneViewInfo& ViewInfo)
{	
	auto Output = Builder.CreateResource<RDAG::ShadowMapTextureArray>()(Input);

	SceneViewInfo ShadowViewInfo = ViewInfo;
	ShadowViewInfo.SceneWidth = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.SceneHeight = ShadowViewInfo.ShadowResolution;
	ShadowViewInfo.DepthFormat = ShadowViewInfo.ShadowFormat;

	for (U32 i = 0; i < ShadowViewInfo.ShadowCascades; i++)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("ShadowMap_DepthRenderPass", DepthRenderPass::Build, ShadowViewInfo),
			Builder.RenameEntry<RDAG::DepthTarget, RDAG::ShadowMapTextureArray>(0, i)
		}(Output);
	}
	return Output;
}