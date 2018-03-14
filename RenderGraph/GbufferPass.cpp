#include "GbufferPass.h"


typename GbufferRenderPass::PassOutputType GbufferRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	Texture2d::Descriptor GbufferDescriptors[RDAG::Gbuffer::ResourceCount];
	for (U32 i = 0; i < RDAG::Gbuffer::ResourceCount; i++)
	{
		GbufferDescriptors[i] = Input.GetInputDescriptor<RDAG::DepthTarget>();
		GbufferDescriptors[i].Name = "GbufferTarget";
		GbufferDescriptors[i].Format = ERenderResourceFormat::ARGB16F;
	}

	return Seq
	(
		Builder.CreateOutputResource<RDAG::Gbuffer>(GbufferDescriptors),
		Builder.QueueRenderAction("GbufferRenderAction", [](RenderContext&, const PassOutputType&)
		{
			//auto DepthTarget = FDataSet::GetMutableResource<RDAG::FDepthTarget>(RndCtx, Self->PassData);
			//(void)DepthTarget;
			//auto GbufferA = FDataSet::GetMutableResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 0);
			//(void)GbufferA;
			//auto GbufferB = FDataSet::GetMutableResource<RDAG::FGbuffer>(RndCtx, Self->PassData, 1);
			//(void)GbufferB;

			//RndCtx.SetRenderPass(Self);
			//RndCtx.BindMutable(DepthTarget);
			//RndCtx.BindMutable(GbufferA);
			//RndCtx.BindMutable(GbufferB);
			//RndCtx.SetRenderPass(nullptr);
		})
	)(Input);
}