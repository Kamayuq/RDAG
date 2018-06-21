#include "CopyTexturePass.h"

typename CopyTexturePass::CopyTextureResult CopyTexturePass::Build(const RenderPassBuilder& Builder, const CopyTextureInput& Input)
{
	return Seq
	{
		//TODO should become a build-in action
		Builder.QueueRenderAction("CopyTextureAction", [](RenderContext& Ctx, const CopyTextureInput&)
		{
			Ctx.Draw("CopyTextureAction");
		})
	}(Input);
}