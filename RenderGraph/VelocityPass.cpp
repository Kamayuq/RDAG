#include "VelocityPass.h"


typename VelocityRenderPass::ReturnType VelocityRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetInputDescriptor<RDAG::DepthTexture>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

#if ASYNC_VELOCITY
	return Async
#else
	return Seq
#endif
	{
		Builder.CreateOutputResource<RDAG::VelocityVectors>({ VelocityDescriptor }),
		Builder.QueueRenderAction("VelocityRenderAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("VelocityRenderAction");
		})
	}(Input);
}