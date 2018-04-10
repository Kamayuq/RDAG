#include "VelocityPass.h"


typename VelocityRenderPass::PassOutputType VelocityRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	auto DepthInfo = Input.GetDescriptor<RDAG::DepthTarget>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

	return Seq
	{
		Builder.CreateResource<RDAG::VelocityVectors>({ VelocityDescriptor }),
		Builder.QueueRenderAction<RDAG::VelocityVectors>("VelocityRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("VelocityRenderAction");
		})
	}(Input);
}