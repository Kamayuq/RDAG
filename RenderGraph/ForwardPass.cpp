#include "ForwardPass.h"


typename ForwardRenderPass::PassOutputType ForwardRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto ForwardInfo = Input.GetInputHandle<RDAG::ForwardRender>();
	(void)ForwardInfo;

	return Seq
	(
		Builder.QueueRenderAction("ForwardRenderAction", [](RenderContext&, const PassOutputType&)
		{
			//(void)Config;
			//auto DepthTarget = FDataSet::GetMutableResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto ForwardTarget = FDataSet::GetMutableResource<RDAG::FForwardResult>(RndCtx, Self->PassData);
			//(void)ForwardTarget;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.BindMutable(ForwardTarget);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}