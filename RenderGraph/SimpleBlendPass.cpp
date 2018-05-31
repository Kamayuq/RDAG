#include "SimpleBlendPass.h"

typename SimpleBlendPass::SimpleBlendResult SimpleBlendPass::Build(const RenderPassBuilder& Builder, const SimpleBlendInput& Input, EBlendMode::Type BlendMode)
{
	(void)BlendMode;
	const Texture2d::Descriptor& BlendSrcInfo = Input.GetDescriptor<RDAG::BlendSource>();
	Texture2d::Descriptor BlendDstDescriptor;
	BlendDstDescriptor.Name = "BlendDestinationRenderTarget";
	BlendDstDescriptor.Format = BlendSrcInfo.Format;
	BlendDstDescriptor.Height = BlendSrcInfo.Height;
	BlendDstDescriptor.Width = BlendSrcInfo.Width;

	using SimpleBlendAction = decltype(std::declval<SimpleBlendInput>().Union(std::declval<SimpleBlendResult>()));
	return Seq
	{
		Builder.CreateResource<RDAG::BlendDest>({ BlendDstDescriptor }),
		Builder.QueueRenderAction("SimpleBlendAction", [](RenderContext& Ctx, const SimpleBlendAction&)
		{
			Ctx.Draw("SimpleBlendAction");
		})
	}(Input);
}