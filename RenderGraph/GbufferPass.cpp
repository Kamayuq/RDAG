#include "GbufferPass.h"

namespace RDAG
{
	SIMPLE_RT_HANDLE(GbufferTarget, GbufferTexture);
}

static constexpr U32 NumGbuffers = 4;

typename GbufferRenderPass::PassOutputType GbufferRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor GbufferDescriptors[NumGbuffers];
	for (U32 i = 0; i < NumGbuffers; i++)
	{
		GbufferDescriptors[i] = Input.GetDescriptor<RDAG::DepthTarget>();
		GbufferDescriptors[i].Name = "GbufferTarget";
		GbufferDescriptors[i].Format = ERenderResourceFormat::ARGB16F;
	}

	using PassActionType = ResourceTable<RDAG::GbufferTarget, RDAG::DepthTarget>;
	return Seq
	{
		Builder.CreateResource<RDAG::GbufferTarget>(GbufferDescriptors),
		Builder.QueueRenderAction("GbufferRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("GbufferRenderAction");
		})
	}(Input);
}