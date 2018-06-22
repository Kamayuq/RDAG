#include "DepthPass.h"
#include "RHI.h"

typename DepthRenderPass::DepthRenderResult DepthRenderPass::Build(const RenderPassBuilder& Builder, const DepthRenderInput& Input, const SceneViewInfo& ViewInfo)
{
	Texture2d::Descriptor DepthDescriptor;
	DepthDescriptor.Name = "DepthRenderTarget";
	DepthDescriptor.Format = ViewInfo.DepthFormat;
	DepthDescriptor.Height = ViewInfo.SceneHeight;
	DepthDescriptor.Width = ViewInfo.SceneWidth;

	return Seq
	{
		Builder.CreateResource<RDAG::DepthTarget>( DepthDescriptor ),
		Builder.QueueRenderAction("DepthRenderAction", [](RenderContext& Ctx, const DepthRenderResult&)
		{
			Ctx.Draw("DepthRenderAction");
		})
	}(Input);
}