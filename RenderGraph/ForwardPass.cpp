#include "ForwardPass.h"


typename ForwardRenderPass::PassOutputType ForwardRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto ForwardInfo = Input.GetHandle<RDAG::ForwardRenderTarget>();
	(void)ForwardInfo;

	return Seq
	{
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext& Ctx, const PassActionType& Resources) -> PassOutputType
		{
			Ctx.Draw("ForwardRenderAction");
			return Resources;
		})
	}(Input);
}