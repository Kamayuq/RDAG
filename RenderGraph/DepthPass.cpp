#include "DepthPass.h"
#include "RHI.h"

typename DepthRenderPass::PassOutputType DepthRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, const SceneViewInfo& ViewInfo)
{
	Texture2d::Descriptor DepthDescriptor;
	DepthDescriptor.Name = "DepthRenderTarget";
	DepthDescriptor.Format = ViewInfo.DepthFormat;
	DepthDescriptor.Height = ViewInfo.SceneHeight;
	DepthDescriptor.Width = ViewInfo.SceneWidth;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.CreateResource<RDAG::DepthTarget>({ DepthDescriptor }),
		Builder.QueueRenderAction("DepthRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("DepthRenderAction");
		})
	}(Input);
}