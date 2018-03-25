#include "ForwardPass.h"


typename ForwardRenderPass::PassOutputType ForwardRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto ForwardInfo = Input.GetInputHandle<RDAG::ForwardRenderTarget>();
	(void)ForwardInfo;

	return Seq
	(
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("ForwardRenderAction");
		})
	)(Input);
}