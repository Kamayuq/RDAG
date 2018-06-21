#include "GbufferPass.h"

namespace RDAG
{
	SIMPLE_RT_HANDLE(GbufferTarget, GbufferTexture);
}

static constexpr U32 NumGbuffers = 4;

typename GbufferRenderPass::GbufferRenderResult GbufferRenderPass::Build(const RenderPassBuilder& Builder, const GbufferRenderInput& Input)
{
	Texture2d::Descriptor GbufferDescriptor;
	GbufferDescriptor = Input.GetDescriptor<RDAG::DepthTarget>();
	GbufferDescriptor.Name = "GbufferTarget";
	GbufferDescriptor.Format = ERenderResourceFormat::ARGB16F;
	GbufferDescriptor.TexSlices = NumGbuffers;

	using GbufferRenderAction = ResourceTable<RDAG::GbufferTarget, RDAG::DepthTarget>;
	return Seq
	{
		Builder.CreateResource<RDAG::GbufferTarget>(GbufferDescriptor),
		Builder.QueueRenderAction("GbufferRenderAction", [](RenderContext& Ctx, const GbufferRenderAction&)
		{
			Ctx.Draw("GbufferRenderAction");
		})
	}(Input);
}