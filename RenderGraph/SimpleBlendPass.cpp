#include "SimpleBlendPass.h"


typename SimpleBlendPass::PassOutputType SimpleBlendPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto BlendSrcInfo = Input.GetInputDescriptor<RDAG::BlendSource>(0);
	Texture2d::Descriptor BlendDstDescriptor;
	BlendDstDescriptor.Name = "BlendDestinationRenderTarget";
	BlendDstDescriptor.Format = BlendSrcInfo.Format;
	BlendDstDescriptor.Height = BlendSrcInfo.Height;
	BlendDstDescriptor.Width = BlendSrcInfo.Width;

	return Seq
	{
		Builder.CreateOutputResource<RDAG::BlendDest>({ BlendDstDescriptor }),
		Builder.QueueRenderAction("SimpleBlendAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("SimpleBlendAction");
		})
	}(Input);
}