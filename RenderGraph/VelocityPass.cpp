#include "VelocityPass.h"


typename VelocityRenderPass::PassOutputType VelocityRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetInputDescriptor<RDAG::DepthTexture>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

	return Seq
	{
		Builder.CreateOutputResource<RDAG::VelocityVectors>({ VelocityDescriptor }),
		Builder.QueueRenderAction("VelocityRenderAction", [](RenderContext& Ctx, const PassOutputType&)
		{
			Ctx.Draw("VelocityRenderAction");
		})
	}(Input);
}