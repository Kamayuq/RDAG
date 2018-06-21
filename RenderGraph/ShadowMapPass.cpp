#include "ShadowMapPass.h"
#include "DepthPass.h"
#include "CopyTexturePass.h"


typename ShadowMapRenderPass::ShadowMapRenderResult ShadowMapRenderPass::Build(const RenderPassBuilder& Builder, const ShadowMapRenderInput& Input, const SceneViewInfo& ViewInfo)
{
	Texture2d::Descriptor ShadowMapArrayDescriptor;
	ShadowMapArrayDescriptor.Width = ViewInfo.ShadowResolution;
	ShadowMapArrayDescriptor.Height = ViewInfo.ShadowResolution;
	ShadowMapArrayDescriptor.Format = ViewInfo.ShadowFormat;
	ShadowMapArrayDescriptor.TexSlices = ViewInfo.ShadowCascades;
	ShadowMapArrayDescriptor.Name = "ShadowMapArray";

	auto Output = Builder.CreateResource<RDAG::ShadowMapTextureArray>(ShadowMapArrayDescriptor)(Input);

	SceneViewInfo ShadowViewInfo = ViewInfo;
	ShadowViewInfo.SceneWidth = ViewInfo.ShadowResolution;
	ShadowViewInfo.SceneHeight = ViewInfo.ShadowResolution;
	ShadowViewInfo.DepthFormat = ViewInfo.ShadowFormat;

	for (U32 i = 0; i < ShadowViewInfo.ShadowCascades; i++)
	{
		Output = Seq
		{
			Builder.BuildRenderPass("ShadowMap_DepthRenderPass", DepthRenderPass::Build, ShadowViewInfo),
			Builder.AssignEntry<RDAG::ShadowMapTextureArray, RDAG::CopyDestination>(i),
			Builder.AssignEntry<RDAG::DepthTexture, RDAG::CopySource>(),
			Builder.BuildRenderPass("CopyShadowDepthSlice", CopyTexturePass::Build),
			Builder.AssignEntry<RDAG::CopyDestination, RDAG::ShadowMapTextureArray>()
		}(Output);
	}
	return Output;
}