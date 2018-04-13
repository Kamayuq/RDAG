#include "ForwardPass.h"


typename ForwardRenderPass::PassOutputType ForwardRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto ForwardInfo = Input.GetHandle<RDAG::ForwardRenderTarget>();
	(void)ForwardInfo;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("ForwardRenderAction");
		})
	}(Input);
}