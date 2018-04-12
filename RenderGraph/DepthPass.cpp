#include "DepthPass.h"
#include "RHI.h"

namespace RDAG
{
	void DepthTexture::OnExecute(ImmediateRenderContext& Ctx, const DepthTexture::ResourceType& Resource) const
	{
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTexture);
		Ctx.BindTexture(Resource);
	}

	void DepthTarget::OnExecute(ImmediateRenderContext& Ctx, const DepthTarget::ResourceType& Resource) const
	{
		Ctx.TransitionResource(Resource, EResourceTransition::DepthTarget);
		Ctx.BindRenderTarget(Resource);
	}
}

typename DepthRenderPass::PassOutputType DepthRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const RDAG::SceneViewInfo& ViewInfo = Input.GetHandle<RDAG::SceneViewInfo>();
	Texture2d::Descriptor DepthDescriptor;
	DepthDescriptor.Name = "DepthRenderTarget";
	DepthDescriptor.Format = ViewInfo.DepthFormat;
	DepthDescriptor.Height = ViewInfo.SceneHeight;
	DepthDescriptor.Width = ViewInfo.SceneWidth;

	return Seq
	{
		Builder.CreateResource<RDAG::DepthTarget>({ DepthDescriptor }),
		Builder.QueueRenderAction("DepthRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("DepthRenderAction");
		})
	}(Input);
}