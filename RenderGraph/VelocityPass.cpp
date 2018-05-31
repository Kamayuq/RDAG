#include "VelocityPass.h"

namespace RDAG
{
	SIMPLE_RT_HANDLE(VelocityVectorTarget, VelocityVectors);
}

typename VelocityRenderPass::PassOutputType VelocityRenderPass::Build(const RenderPassBuilder& Builder, const PassInputType& Input)
{
	const Texture2d::Descriptor& DepthInfo = Input.GetDescriptor<RDAG::DepthTarget>();
	Texture2d::Descriptor VelocityDescriptor;
	VelocityDescriptor.Name = "VelocityRenderTarget";
	VelocityDescriptor.Format = ERenderResourceFormat::RG16F;
	VelocityDescriptor.Height = DepthInfo.Height;
	VelocityDescriptor.Width = DepthInfo.Width;

	using PassActionType = ResourceTable<RDAG::VelocityVectorTarget, RDAG::DepthTexture>;
	return Seq
	{
		Builder.CreateResource<RDAG::VelocityVectors>({ VelocityDescriptor }),
		Builder.QueueRenderAction("VelocityRenderAction", [](RenderContext& Ctx, const PassActionType&)
		{
			Ctx.Draw("VelocityRenderAction");
		})
	}(Input);
}