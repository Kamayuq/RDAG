#include "GbufferPass.h"

namespace RDAG
{
	struct GbufferTarget : RendertargetResourceHandle<GbufferTexture>
	{
		static constexpr const U32 ResourceCount = 4;
		static constexpr const char* Name = "GbufferTarget";

		explicit GbufferTarget() {}
	};
}

typename GbufferRenderPass::PassOutputType GbufferRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor GbufferDescriptors[RDAG::GbufferTarget::ResourceCount];
	for (U32 i = 0; i < RDAG::GbufferTarget::ResourceCount; i++)
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