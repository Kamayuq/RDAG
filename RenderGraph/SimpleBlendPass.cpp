#include "SimpleBlendPass.h"

typename SimpleBlendPass::PassOutputType SimpleBlendPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input, EBlendMode::Type BlendMode)
{
	auto BlendSrcInfo = Input.GetDescriptor<RDAG::BlendSource>();
	Texture2d::Descriptor BlendDstDescriptor;
	BlendDstDescriptor.Name = "BlendDestinationRenderTarget";
	BlendDstDescriptor.Format = BlendSrcInfo.Format;
	BlendDstDescriptor.Height = BlendSrcInfo.Height;
	BlendDstDescriptor.Width = BlendSrcInfo.Width;

	using PassActionType = decltype(std::declval<PassInputType>().Union(std::declval<PassOutputType>()));
	return Seq
	{
		Builder.CreateResource<RDAG::BlendDest>({ BlendDstDescriptor }),
		Builder.QueueRenderAction("SimpleBlendAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("SimpleBlendAction");
		})
	}(Input);
}