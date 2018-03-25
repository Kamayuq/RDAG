#include "GbufferPass.h"


typename GbufferRenderPass::PassOutputType GbufferRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor GbufferDescriptors[RDAG::GbufferTarget::ResourceCount];
	for (U32 i = 0; i < RDAG::GbufferTarget::ResourceCount; i++)
	{
		GbufferDescriptors[i] = Input.GetInputDescriptor<RDAG::DepthTarget>();
		GbufferDescriptors[i].Name = "GbufferTarget";
		GbufferDescriptors[i].Format = ERenderResourceFormat::ARGB16F;
	}

	return Seq
	(
		Builder.CreateOutputResource<RDAG::GbufferTarget>(GbufferDescriptors),
		Builder.QueueRenderAction("GbufferRenderAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("GbufferRenderAction");
		})
	)(Input);
}