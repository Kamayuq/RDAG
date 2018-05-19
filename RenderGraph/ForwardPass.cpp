#include "ForwardPass.h"


typename ForwardRenderPass::PassOutputType ForwardRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, ESortOrder::Type SortOrder)
{
	(void)SortOrder;
	using PassActionType = ResourceTable<RDAG::ForwardRenderTarget, RDAG::DepthTarget>;
	return Seq
	{
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("ForwardRenderAction");
		})
	}(Input);
}