#include "ForwardPass.h"


typename ForwardRenderPass::ForwardRenderResult ForwardRenderPass::Build(const RenderPassBuilder& Builder, const ForwardRenderInput& Input, ESortOrder::Type SortOrder)
{
	(void)SortOrder;
	using ForwardRenderAction = ResourceTable<RDAG::ForwardRenderTarget, RDAG::DepthTarget>;
	return Seq
	{
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext& Ctx, const ForwardRenderAction&)
		{
			Ctx.Draw("ForwardRenderAction");
		})
	}(Input);
}